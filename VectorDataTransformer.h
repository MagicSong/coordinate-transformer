#pragma once
class VectorDataTransformer
{
public:
	VectorDataTransformer();
	~VectorDataTransformer();
	void ReProject(string sourceFile, string desFileName,OGRSpatialReference* sourceProj, OGRSpatialReference* desProj);
	string SourceFileName;
private:
	unordered_map<string, string>_FormatMap;
	const string FORMATS_FILE = "formats.txt";
};

