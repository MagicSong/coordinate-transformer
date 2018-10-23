#include "stdafx.h"
#include "RasterDataTransformer.h"
#include "PointTransformer.h"


RasterDataTransformer::RasterDataTransformer()
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
}


RasterDataTransformer::~RasterDataTransformer()
{
}
int RasterDataTransformer::Reproject(string sourceFile, string outputFile, OGRSpatialReference * From, OGRSpatialReference * To, OGRSpatialReference * GCPFrom = nullptr, OGRSpatialReference * GCPTo = nullptr, _Matrix* M = nullptr)
{
	char* fromProj, *toProj;
	From->exportToProj4(&fromProj);
	To->exportToProj4(&toProj);
	if (From->IsSameGeogCS(To))
	{
		string cmd = "gdalwarp -s_srs \"" + string(fromProj) + "\" -t_srs \"" + string(toProj) + "\" -overwrite  --config GDAL_FILENAME_IS_UTF8 NO \"" + sourceFile + "\" \"" + outputFile + "\"";
		//cout << cmd << endl;
		system(cmd.c_str());
	}
	else
	{
		if (M == NULL)
		{
			cout << "不同的椭球体转换需要控制点！" << endl;
			return -1;
		}
		GDALDataset* sourceDs = (GDALDataset*)GDALOpen(sourceFile.c_str(), GA_ReadOnly);
		TransformEllipsod(sourceDs, CPLGetExtension(outputFile.c_str()), outputFile.c_str(), GRA_Bilinear, From, To, GCPFrom,GCPTo,M);
	}
	return 0;
}
int RasterDataTransformer::TransformEllipsod(GDALDataset * sourceDs, const char* pszFormat, const char* outputFileName, GDALResampleAlg resample,OGRSpatialReference * From, OGRSpatialReference* To, OGRSpatialReference * GCPFrom, OGRSpatialReference * GCPTo, _Matrix * M)
{
	double adfDstGeoTransform[6];
	double dbX[4];
	double dbY[4];
	double dbZ[4] = { 0,0,0,0 };
	int nXsize = sourceDs->GetRasterXSize();
	int nYsize = sourceDs->GetRasterYSize();
	sourceDs->GetGeoTransform(adfDstGeoTransform);
	dbX[0] = adfDstGeoTransform[0];    //左上角点坐标
	dbY[0] = adfDstGeoTransform[3];

	//右上角坐标
	dbX[1] = adfDstGeoTransform[0] + nXsize*adfDstGeoTransform[1];
	dbY[1] = adfDstGeoTransform[3];

	//右下角点坐标
	dbX[2] = adfDstGeoTransform[0] + nXsize*adfDstGeoTransform[1] + nYsize*adfDstGeoTransform[2];
	dbY[2] = adfDstGeoTransform[3] + nXsize*adfDstGeoTransform[4] + nYsize*adfDstGeoTransform[5];

	//左下角坐标
	dbX[3] = adfDstGeoTransform[0];
	dbY[3] = adfDstGeoTransform[3] + nYsize*adfDstGeoTransform[5];

	PointTransformer* gcppt = PointTransformer::CreateTransfromer(From, To, GCPFrom, GCPTo, M);
	gcppt->Project(dbX, dbY, 4);
	int nGCPCount = 4;
	GDAL_GCP gcpPoints[4];
	for (int i = 0; i < nGCPCount; i++)
	{
		gcpPoints[i].dfGCPX = dbX[i];
		gcpPoints[i].dfGCPY = dbY[i];
		gcpPoints[i].dfGCPZ = 0;
		char a[2];
		sprintf_s(a, "%d", i);
		gcpPoints[i].pszId = a;
		gcpPoints[i].pszInfo = "";
	}
	gcpPoints[0].dfGCPLine = 0; gcpPoints[0].dfGCPPixel = 0;
	gcpPoints[1].dfGCPLine = 0; gcpPoints[1].dfGCPPixel = nXsize - 1;
	gcpPoints[2].dfGCPLine = nYsize - 1; gcpPoints[2].dfGCPPixel = nXsize - 1;
	gcpPoints[3].dfGCPLine = nYsize - 1; gcpPoints[3].dfGCPPixel = 0;
	// 建立变换选项 
	GDALWarpOptions* psWarpOptions = GDALCreateWarpOptions();
	psWarpOptions->hSrcDS = sourceDs;

	int nBandCount = GDALGetRasterCount(sourceDs);
	psWarpOptions->nBandCount = nBandCount;
	psWarpOptions->panSrcBands =
		(int *)CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
	for (int i = 0; i < nBandCount; i++)
	{
		psWarpOptions->panSrcBands[i] = i + 1;
	}
	psWarpOptions->panDstBands =
		(int *)CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
	for (int i = 0; i < nBandCount; i++)
	{
		psWarpOptions->panDstBands[i] = i + 1;
	}

	psWarpOptions->pfnProgress = GDALTermProgress;
	psWarpOptions->eResampleAlg = resample;

	// 创建重投影变换函数    
	psWarpOptions->pTransformerArg = GDALCreateGCPTransformer(nGCPCount, gcpPoints, 1, 0);
	psWarpOptions->pfnTransformer = GDALGCPTransform;

	double adfDstGeoTransform[6]; 
	int nPixels = 0, nLines = 0; 
	GDALSuggestedWarpOutput(psWarpOptions->hSrcDS, GDALGCPTransform, psWarpOptions->pTransformerArg, adfDstGeoTransform, &nPixels, &nLines);
	GDALDriver* hDriver= (GDALDriver*)GDALGetDriverByName(pszFormat);
	GDALDataset* hDstDS = (GDALDataset*)GDALCreate(hDriver,outputFileName, nPixels, nLines, GDALGetRasterCount(sourceDs), sourceDs->GetRasterBand(0)->GetRasterDataType(),NULL)
	CPLAssert(hDstDS != NULL);
	// 写入投影
	psWarpOptions->hDstDS = hDstDS;
	char* proj;
	GCPTo->exportToWkt(&proj);
	hDstDS->SetProjection(proj);

	// 复制颜色表，如果有的话    
	GDALColorTableH hCT;
	hCT = GDALGetRasterColorTable(GDALGetRasterBand(sourceDs, 1));
	if (hCT != NULL)
		GDALSetRasterColorTable(GDALGetRasterBand(hDstDS, 1), hCT);

   
	

	// 初始化并且执行变换操作    
	GDALWarpOperation oOperation;
	oOperation.Initialize(psWarpOptions);
	oOperation.ChunkAndWarpImage(0, 0, GDALGetRasterXSize(hDstDS), GDALGetRasterYSize(hDstDS));
	GDALDestroyGCPTransformer(psWarpOptions->pTransformerArg);
	GDALDestroyWarpOptions(psWarpOptions);
	GDALClose(hDstDS);
	GDALClose(sourceDs);
	return 0;
}
