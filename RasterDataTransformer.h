#pragma once
class RasterDataTransformer
{
public:
	RasterDataTransformer();
	~RasterDataTransformer();

	int Reproject(string sourceFile,string outputFile,OGRSpatialReference* From,OGRSpatialReference* To, OGRSpatialReference * GCPFrom=nullptr, OGRSpatialReference * GCPTo = nullptr, _Matrix* M = nullptr);
	int TransformEllipsod(GDALDataset * sourceDs, const char* pszFormat, const char* outputFileName, GDALResampleAlg resample, OGRSpatialReference * From, OGRSpatialReference* To, OGRSpatialReference * GCPFrom, OGRSpatialReference * GCPTo, _Matrix * M);

};

