#pragma once
class CoordinateBuilder
{
public:
	CoordinateBuilder(char* projFiles);
	CoordinateBuilder();
	~CoordinateBuilder();
	// �������ɸ�˹ͶӰ�ַ���
	OGRSpatialReference* BulidGaussProjection(double cenLon, string geo, bool appendDegree=false);
	map<string, string> projMap;
private:
	char* projFiles;
};

