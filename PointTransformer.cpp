#include "stdafx.h"
#include "PointTransformer.h"
#include "_Matrix.h"
#include "CoordinateBuilder.h"

PointTransformer::PointTransformer()
{

}


PointTransformer* PointTransformer::CreateTransformer(char * FromWKT, char * ToWKT)
{
	OGRSpatialReference*  fromProj = new OGRSpatialReference();
	OGRSpatialReference* toProj = new OGRSpatialReference();
	fromProj->importFromWkt(&FromWKT);
	toProj->importFromWkt(&ToWKT);
	//OGRSpatialReference* toGeoS = toProj->CloneGeogCS();
	PointTransformer* pt = new PointTransformer();
	OGRCoordinateTransformation* projTransformer = OGRCreateCoordinateTransformation(fromProj, toProj);
	if (projTransformer == NULL)
	{
		cout << "����ͶӰת��ʧ��" << endl;
		return NULL;
	}
	pt->projTransformer = projTransformer;
	return pt;
}

PointTransformer * PointTransformer::CreateTransformer(OGRSpatialReference * From, OGRSpatialReference * To)
{
	OGRCoordinateTransformation* projTransformer = OGRCreateCoordinateTransformation(From, To);
	if (projTransformer == NULL)
	{
		cout << "����ͶӰת��ʧ��" << endl;
		return NULL;
	}
	PointTransformer* pt = new PointTransformer();
	pt->projTransformer = projTransformer;
	return pt;
}

PointTransformer * PointTransformer::CreateTransfromer(OGRSpatialReference * From, OGRSpatialReference* To, OGRSpatialReference * GCPFrom, OGRSpatialReference * GCPTo, _Matrix * m)
{
	PointTransformer* pt = new PointTransformer();
	pt->M = m;
	if (!From->IsSame(GCPFrom))
	{
		pt->fromTransformer = OGRCreateCoordinateTransformation(From, GCPFrom);
		if (pt->fromTransformer == nullptr)
		{
			cout << "��������ԴͶӰ����Ƶ�ԴͶӰ�޷�ת����" << endl;
			return nullptr;
		}
	}
	if (!To->IsSame(GCPTo))
	{
		pt->toTransformer = OGRCreateCoordinateTransformation(To, GCPTo);
		if (pt->toTransformer == nullptr)
		{
			cout << "��������Ŀ��ͶӰ����Ƶ�Ŀ��ͶӰ�޷�ת����" << endl;
			return nullptr;
		}
	}
	return pt;
}

_Matrix * PointTransformer::GetTransMatrix(string sourceFile, string desFile)
{
	fstream filex1;
	filex1.open(sourceFile, ios::in);
	//filex1.open("D:\\My University\\�о�������\\�о�����Ŀ\\�����Ƽ���ѧͶӰ�任��Ŀ\\Data\\��Դ80.txt", ios::in);
	int count = 0;
	filex1 >> count;
	//cout << count << endl;
	double* x80 = new double[count];
	double* y80 = new double[count];
	double* x2000 = new double[count];
	double* y2000 = new double[count];
	for (int i = 0; i < count; i++)
	{
		int id;
		string y;
		filex1 >> id >> x80[i] >> y;
		y80[i] = atof(y.substr(2).c_str());
	}
	filex1.close();
	//cout << "2000���꿪ʼ��ȡ" << endl;
	filex1.open(desFile, ios::in);
	filex1 >> count;
	//cout << count << endl;
	for (int i = 0; i < count; i++)
	{
		int id; string y;
		filex1 >> id >> x2000[i] >> y;
		y2000[i] = atof(y.substr(2).c_str());
	}
	filex1.close();
	//���￪ʼ�������
	_Matrix* B = new _Matrix(count * 2, 4);
	_Matrix* L = new _Matrix(2 * count, 1);
	B->init_matrix(); L->init_matrix();
	for (int i = 0; i < count; i++)
	{
		B->write(2 * i, 0, 1);
		B->write(2 * i, 1, 0);
		B->write(2 * i, 2, x80[i]);
		B->write(2 * i, 3, -y80[i]);
		B->write(2 * i + 1, 0, 0);
		B->write(2 * i + 1, 1, 1);
		B->write(2 * i + 1, 2, y80[i]);
		B->write(2 * i + 1, 3, x80[i]);
		L->write(2 * i, 0, x2000[i]);
		L->write(2 * i + 1, 0, y2000[i]);
	}
	//printf_matrix(B);
	_Matrix* temp = new _Matrix(4, 2 * count);
	temp->init_matrix();
	_Matrix_Calc m_c;
	m_c.transpos(B, temp);//��ת��
	_Matrix*temp2 = new  _Matrix(4, 4);
	temp2->init_matrix();
	m_c.multiply(temp, B, temp2);//���
	_Matrix*temp3 = new  _Matrix(4, 4);
	temp3->init_matrix();
	m_c.inverse(temp2, temp3);
	_Matrix*temp4 = new  _Matrix(4, 2 * count);
	temp4->init_matrix();
	m_c.multiply(temp3, temp, temp4);//����b��ת��
	_Matrix* M;
	m_c.multiply(temp4, L, M);
	temp->free_matrix(); temp2->free_matrix(); temp3->free_matrix(); temp4->free_matrix();
	return M;
}

PointTransformer::~PointTransformer()
{
}

int PointTransformer::Project(double * x, double * y, int count)
{
	if (M == nullptr)
		projTransformer->Transform(count, x, y);
	else
	{
		//���Ƶ�ģʽ
		if (fromTransformer != nullptr)
		{
			fromTransformer->Transform(count, x, y);
		}
		GCPTransformer(x, y, count);//���ݿ��Ƶ�ת��
		if (toTransformer != nullptr)
		{
			toTransformer->Transform(count, x, y);
		}
	}
	return 0;
}

// �ÿ��Ƶ���ת������
int PointTransformer::GCPTransformer(double* x, double* y, int count)
{
	double m = sqrt(M->read(0, 2)*M->read(0, 2) + M->read(0, 3)*M->read(0, 3));
	double cosa = M->read(0, 2) / m;
	double sina = M->read(0, 3) / m;
	for (int i = 0; i < count; i++) {
		_Matrix* xyM80 = new _Matrix(2, 4);
		xyM80->init_matrix();
		xyM80->write(0, 0, 1); xyM80->write(0, 1, 0); xyM80->write(0, 2, x[i]); xyM80->write(0, 3, -y[i]);
		xyM80->write(1, 0, 0); xyM80->write(1, 1, 1); xyM80->write(1, 2, y[i]); xyM80->write(1, 3, x[i]);
		_Matrix* xy2000 = new _Matrix(2, 1);
		xy2000->init_matrix();
		_Matrix_Calc m_c;
		m_c.multiply(xyM80, M, xy2000);
		x[i] = xy2000->read(0, 0);
		y[i] = xy2000->read(1, 0);
	}
	return 0;
}
