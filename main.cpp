#include "GL\glew.h"
#include "GL\freeglut.h"
#include <iostream>  
#include <vector>
#include <cassert>

#include "textfile.h" 

using namespace std;

typedef struct  Point2D
{
public:
	Point2D()
	{
		x = 0.0;
		y = 0.0;
	}

	Point2D(float mx, float my)
	{
		x = mx;
		y = my;
	}

	Point2D(const Point2D& pt)
	{
		x = pt.x;
		y = pt.y;
	}

	Point2D operator -(const Point2D& pt)
	{
		return Point2D(this->x - pt.x, this->y - pt.y);
	}

	float x;
	float y;
} Point2D;

typedef struct Point3D
{
	Point3D()
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
	}
	Point3D(float mx, float my, float mz)
	{
		x = mx;
		y = my;
		z = mz;
	}
	float x;
	float y;
	float z;
} Point3D;

typedef struct Color
{
	Color(float mr, float mg, float mb, float ma)
	{
		r = mr;
		g = mg;
		b = mb;
		a = ma;
	}
	float r;
	float g;
	float b;
	float a;
} Color;


typedef struct vec2
{
	vec2(float mx, float my)
	{
		x = mx;
		y = my;
	}
	float x;
	float y;
} vec2;

struct vec4
{
	vec4(float mx, float my, float mz, float mw)
	{
		x = mx;
		y = my;
		z = mz;
		w = mw;
	}
	float x;
	float y;
	float z;
	float w;
};

//////////////////////////////////////////////////////////////////////
//定义Uniform变量
vec4 m_color(1.0, 0.0, 0.0, 1.0);
float m_lineWidth = 19.0;
float m_antialias = 1.0;
vec2 m_lineCaps = vec2(2.0, 2.0);
float m_lineJoin = 1.0;
float m_miter_limit = 10.0;
float m_length = 0.0;
float m_bClosed = 0.0;


//buffer数据
//点集
vector<vec2> m_pts;

vector<float> m_ps;

//段长
vector<vec2> m_segment;

//角度
vector<vec2> m_angles;

//正切
vector<vec4> m_tangents;

//纹理坐标
vector<vec2> m_texcoord;

//索引集合
vector<unsigned short> m_index;
//////////////////////////////////////////////////////////////////////




GLuint vShader, fShader;//顶点着色器对象  

GLuint vaoHandle;//vertex array object  

GLfloat xPos = 0.0f;
GLfloat yPos = 0.0f;

GLfloat xScale = 1.0f;

GLfloat x = 100.0;

int mouseX = 0, mouseY = 0;

GLuint programHandle = 0;
typedef float M3DMatrix44f[16];
typedef float	M3DMatrix33f[9];

//投影矩阵
M3DMatrix44f projMatrix;

#define M3D_PI 3.14159265358979323846

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

inline void m3dExtractRotationMatrix33(M3DMatrix33f dst, const M3DMatrix44f src)
{
	memcpy(dst, src, sizeof(float)* 3); // X column
	memcpy(dst + 3, src + 4, sizeof(float)* 3); // Y column
	memcpy(dst + 6, src + 8, sizeof(float)* 3); // Z column

	double sqrtLength0 = sqrtf(dst[0] * dst[0] + dst[1] * dst[1] + dst[2] * dst[2]);
	dst[0] /= sqrtLength0;
	dst[1] /= sqrtLength0;
	dst[2] /= sqrtLength0;

	double sqrtLength3 = sqrtf(dst[3] * dst[3] + dst[4] * dst[4] + dst[5] * dst[5]);
	dst[3] /= sqrtLength3;
	dst[4] /= sqrtLength3;
	dst[5] /= sqrtLength3;

	double sqrtLength6 = sqrtf(dst[6] * dst[6] + dst[7] * dst[7] + dst[8] * dst[8]);
	dst[6] /= sqrtLength6;
	dst[7] /= sqrtLength6;
	dst[8] /= sqrtLength6;


}

///////////////////////////////////////////////////////////////////////////////
void m3dLoadIdentity44(M3DMatrix44f m)
{
	// Don't be fooled, this is still column major
	static M3DMatrix44f	identity = { 1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f };

	memcpy(m, identity, sizeof(M3DMatrix44f));
}

inline void m3dTranslationMatrix44(M3DMatrix44f m, float x, float y, float z)
{
	m3dLoadIdentity44(m); m[12] = x; m[13] = y; m[14] = z;
}

inline void m3dScaleMatrix44(M3DMatrix44f m, float x, float y, float z)
{
	m3dLoadIdentity44(m); m[0] = x; m[5] = y; m[10] = z;
}

void m3dRotationMatrix44(M3DMatrix44f m, float angle, float x, float y, float z)
{
	float mag, s, c;
	float xx, yy, zz, xy, yz, zx, xs ,ys, zs, one_c;

	s = float(sin(angle));
	c = float(cos(angle));

	mag = float(sqrt(x*x + y*y + z*z));

	// Identity matrix
	if (mag == 0.0f) {
		m3dLoadIdentity44(m);
		return;
	}

	// Rotation matrix is normalized
	x /= mag;
	y /= mag;
	z /= mag;

#define M(row,col)  m[col*4+row]

	xx = x * x;
	yy = y * y;
	zz = z * z;
	xy = x * y;
	yz = y * z;
	zx = z * x;
	xs = x * s;
	ys = y * s;
	zs = z * s;
	one_c = 1.0f - c;

	M(0, 0) = (one_c * xx) + c;
	M(0, 1) = (one_c * xy) - zs;
	M(0, 2) = (one_c * zx) + ys;
	M(0, 3) = 0.0f;

	M(1, 0) = (one_c * xy) + zs;
	M(1, 1) = (one_c * yy) + c;
	M(1, 2) = (one_c * yz) - xs;
	M(1, 3) = 0.0f;

	M(2, 0) = (one_c * zx) - ys;
	M(2, 1) = (one_c * yz) + xs;
	M(2, 2) = (one_c * zz) + c;
	M(2, 3) = 0.0f;

	M(3, 0) = 0.0f;
	M(3, 1) = 0.0f;
	M(3, 2) = 0.0f;
	M(3, 3) = 1.0f;

#undef M
}

// Multiply two 4x4 matricies
void m3dMatrixMultiply44(M3DMatrix44f product, const M3DMatrix44f a, const M3DMatrix44f b)
{
	for (int i = 0; i < 4; i++) {
		float ai0 = A(i, 0), ai1 = A(i, 1), ai2 = A(i, 2), ai3 = A(i, 3);
		P(i, 0) = ai0 * B(0, 0) + ai1 * B(1, 0) + ai2 * B(2, 0) + ai3 * B(3, 0);
		P(i, 1) = ai0 * B(0, 1) + ai1 * B(1, 1) + ai2 * B(2, 1) + ai3 * B(3, 1);
		P(i, 2) = ai0 * B(0, 2) + ai1 * B(1, 2) + ai2 * B(2, 2) + ai3 * B(3, 2);
		P(i, 3) = ai0 * B(0, 3) + ai1 * B(1, 3) + ai2 * B(2, 3) + ai3 * B(3, 3);
	}
}

void SetPerspective(float fFov, float fAspect, float fNear, float fFar)
{
	float xmin, xmax, ymin, ymax;       // Dimensions of near clipping plane
	float xFmin, xFmax, yFmin, yFmax;   // Dimensions of far clipping plane

	// Do the Math for the near clipping plane
	ymax = fNear * float(tan(fFov * M3D_PI / 360.0));
	ymin = -ymax;
	xmin = ymin * fAspect;
	xmax = -xmin;

	// Construct the projection matrix
	m3dLoadIdentity44(projMatrix);
	projMatrix[0] = (2.0f * fNear) / (xmax - xmin);
	projMatrix[5] = (2.0f * fNear) / (ymax - ymin);
	projMatrix[8] = (xmax + xmin) / (xmax - xmin);
	projMatrix[9] = (ymax + ymin) / (ymax - ymin);
	projMatrix[10] = -((fFar + fNear) / (fFar - fNear));
	projMatrix[11] = -1.0f;
	projMatrix[14] = -((2.0f * fFar * fNear) / (fFar - fNear));
	projMatrix[15] = 0.0f;
}

void InitData(vector<Point2D>& positions)
{
	//线清空数据
	m_pts.clear();
	m_segment.clear();
	m_angles.clear();
	m_tangents.clear();
	m_texcoord.clear();
	m_index.clear();

	int nPts = positions.size();
	float dx = positions[0].x - positions[nPts-1].x;
	float dy = positions[0].y - positions[nPts-1].y;

	float d = sqrt(dx *dx + dy * dy);

	//如果闭合，必须确保首尾点相同
	if (m_bClosed && (d >1e-10))
	{
		positions.push_back(positions[0]);
	}

	//点个数
	int nCount = positions.size();
	for (int i = 0; i != nCount; ++i)
	{
		m_pts.push_back(vec2(positions[i].x, positions[i].y));
		m_segment.push_back(vec2(0.0, 0.0));
		m_angles.push_back(vec2(0.0, 0.0));
		m_tangents.push_back(vec4(0.0, 0.0, 0.0, 0.0));
		m_texcoord.push_back(vec2(0.0, 0.0));
	}

	//计算切角和法向量
	vector<Point2D> T;
	vector<float> N;
	Point2D tmpPt;
	for(int i = 1; i != nCount; ++i)
	{
		tmpPt = Point2D(positions[i] - positions[i-1]);
		T.push_back(tmpPt);
		N.push_back(sqrt(tmpPt.x*tmpPt.x + tmpPt.y*tmpPt.y));
	}

	for (int i = 1; i != nCount; ++i)
	{
		m_tangents[i].x = T[i-1].x;
		m_tangents[i].y = T[i-1].y;
	}

	if (m_bClosed)
	{
		m_tangents[0].x = T[T.size()-1].x;
		m_tangents[0].y = T[T.size()-1].y;
	}
	else
	{
		m_tangents[0].x = T[0].x;
		m_tangents[0].y = T[0].y;
	}

	//初始化m_tangents
	for (int i = 0; i != (nCount-1); ++i)
	{
		m_tangents[i].z = T[i].x;
		m_tangents[i].w = T[i].y;
	}

	if (m_bClosed)
	{
		m_tangents[nCount-1].z = T[0].x;
		m_tangents[nCount-1].w = T[0].y;
	}
	else
	{
		m_tangents[nCount-1].z = T[T.size()-1].x;
		m_tangents[nCount-1].w = T[T.size()-1].y;
	}


	//计算角度
	vector<vec2> T1, T2;
	for (int i = 0; i != m_pts.size(); ++i)
	{
		T1.push_back(vec2(m_tangents[i].x, m_tangents[i].y));
		T2.push_back(vec2(m_tangents[i].z, m_tangents[i].w));
	}

	vector<float> A;
	for (int i = 0; i != T1.size(); ++i)
	{
		A.push_back(atan2(T1[i].x * T2[i].y - T1[i].y * T2[i].x,
			T1[i].x * T2[i].x + T1[i].y * T2[i].y));
	}

	for (int i = 0; i !=(nCount-1); ++i)
	{
		m_angles[i].x = A[i];
		m_angles[i].y = A[i+1];
	}


	vector<float> L;
	float tmpN = 0.0;
	for (int i = 0; i != N.size(); ++i)
	{
		tmpN += N[i];
		L.push_back(tmpN);
	}

	for (int i = 0; i != (nCount-1); ++i)
	{
		m_segment[i+1].x = L[i];
		m_segment[i].y = L[i];
	}

	//重复第1个到倒数第二个
	vector<vec2> finalSegment, finalPts, finalAngles, finalTexCoord;
	vector<vec4> finalTangents;
	finalPts.push_back(m_pts[0]);
	finalAngles.push_back(m_angles[0]);
	finalSegment.push_back(m_segment[0]);
	finalTangents.push_back(m_tangents[0]);
	finalTexCoord.push_back(m_texcoord[0]);
	for (int i=1; i != (nCount-1); ++i)
	{
		finalPts.push_back(m_pts[i]);
		finalPts.push_back(m_pts[i]);
		finalAngles.push_back(m_angles[i]);
		finalAngles.push_back(m_angles[i]);
		finalSegment.push_back(m_segment[i]);
		finalSegment.push_back(m_segment[i]);
		finalTangents.push_back(m_tangents[i]);
		finalTangents.push_back(m_tangents[i]);
		finalTexCoord.push_back(m_texcoord[i]);
		finalTexCoord.push_back(m_texcoord[i]);
	}
	finalPts.push_back(m_pts[nCount-1]);
	finalSegment.push_back(m_segment[nCount-1]);
	finalAngles.push_back(m_angles[nCount-1]);
	finalTangents.push_back(m_tangents[nCount-1]);
	finalTexCoord.push_back(m_texcoord[nCount-1]);

	vector<vec2> tmpSegment, tmpAngles;
	for (int i = 0; i != finalSegment.size(); ++i)
	{
		tmpSegment.push_back(finalSegment[i]);
		tmpAngles.push_back(finalAngles[i]);
	}

	for (int i = 1; i != finalSegment.size(); ++i)
	{
		finalSegment[i] = tmpSegment[i-1];
		finalAngles[i] = tmpAngles[i-1];
	}

	for (int i = 0; i != finalPts.size(); ++i)
	{
		if ((i%2) == 0)		//偶数
		{
			finalTexCoord[i] = vec2(-1.0, -1.0);
		}
		else
		{
			finalTexCoord[i] = vec2(1.0, 1.0);
		}
	}

	vector<vec2> inSegment, inPts, inAngles, inTexcoord;
	vector<vec4> inTangents;
	for (int i = 0; i != finalPts.size(); ++i)
	{
		inPts.push_back(finalPts[i]);
		inPts.push_back(finalPts[i]);
		inSegment.push_back(finalSegment[i]);
		inSegment.push_back(finalSegment[i]);
		inAngles.push_back(finalAngles[i]);
		inAngles.push_back(finalAngles[i]);
		inTangents.push_back(finalTangents[i]);
		inTangents.push_back(finalTangents[i]);
		inTexcoord.push_back(finalTexCoord[i]);
		inTexcoord.push_back(finalTexCoord[i]);
	}


	for (int i = 0; i != inTexcoord.size(); ++i)
	{
		if (i %2 == 0)
		{
			inTexcoord[i].y = -1.0;
		}
		else
		{
			inTexcoord[i].y = 1.0;
		}
	}

	for (int i = 0; i != (nCount-1); ++i)
	{
		m_index.push_back(4 * i + 0);
		m_index.push_back(4 * i + 1);
		m_index.push_back(4 * i + 2);
		m_index.push_back(4 * i + 1);
		m_index.push_back(4 * i + 2);
		m_index.push_back(4 * i + 3);
	}

	//顶点
	m_pts.clear();
	m_angles.clear();
	m_tangents.clear();
	m_segment.clear();
	m_texcoord.clear();
	for (int i = 0; i != inPts.size(); ++i)
	{
		m_pts.push_back(inPts[i]);
		m_angles.push_back(inAngles[i]);
		m_tangents.push_back(inTangents[i]);
		m_segment.push_back(inSegment[i]);
		m_texcoord.push_back(inTexcoord[i]);
	}
	m_length = L[L.size()-1];
}

void initShader(const char *VShaderFile, const char *FShaderFile)
{
	//1、查看GLSL和OpenGL的版本  
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *glslVersion =
		glGetString(GL_SHADING_LANGUAGE_VERSION);
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	cout << "GL Vendor    :" << vendor << endl;
	cout << "GL Renderer  : " << renderer << endl;
	cout << "GL Version (string)  : " << version << endl;
	cout << "GL Version (integer) : " << major << "." << minor << endl;
	cout << "GLSL Version : " << glslVersion << endl;

	//2、编译着色器  
	//创建着色器对象：顶点着色器  
	vShader = glCreateShader(GL_VERTEX_SHADER);
	//错误检测  
	if (0 == vShader)
	{
		cerr << "ERROR : Create vertex shader failed" << endl;
		exit(1);
	}

	//把着色器源代码和着色器对象相关联  
	const GLchar *vShaderCode = textFileRead(VShaderFile);
	glShaderSource(vShader, 1, &vShaderCode, NULL);

	//编译着色器对象  
	glCompileShader(vShader);


	//检查编译是否成功  
	GLint compileResult;
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &compileResult);
	if (GL_FALSE == compileResult)
	{
		GLint logLen;
		//得到编译日志长度  
		glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char *log = (char *)malloc(logLen);
			GLsizei written;
			//得到日志信息并输出  
			glGetShaderInfoLog(vShader, logLen, &written, log);
			cerr << "vertex shader compile log : " << endl;
			cerr << log << endl;
			free(log);//释放空间  
		}
	}

	//创建着色器对象：片断着色器  
	fShader = glCreateShader(GL_FRAGMENT_SHADER);
	//错误检测  
	if (0 == fShader)
	{
		cerr << "ERROR : Create fragment shader failed" << endl;
		exit(1);
	}

	//把着色器源代码和着色器对象相关联  
	const GLchar *fShaderCode = textFileRead(FShaderFile);
	glShaderSource(fShader, 1, &fShaderCode, NULL);

	//编译着色器对象  
	glCompileShader(fShader);

	//检查编译是否成功  
	glGetShaderiv(fShader, GL_COMPILE_STATUS, &compileResult);
	if (GL_FALSE == compileResult)
	{
		GLint logLen;
		//得到编译日志长度  
		glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char *log = (char *)malloc(logLen);
			GLsizei written;
			//得到日志信息并输出  
			glGetShaderInfoLog(fShader, logLen, &written, log);
			cerr << "fragment shader compile log : " << endl;
			cerr << log << endl;
			free(log);//释放空间  
		}
	}

	//3、链接着色器对象  
	//创建着色器程序  
	programHandle = glCreateProgram();
	if (!programHandle)
	{
		cerr << "ERROR : create program failed" << endl;
		exit(1);
	}
	//将着色器程序链接到所创建的程序中  
	glAttachShader(programHandle, vShader);
	glAttachShader(programHandle, fShader);
	assert(glGetError() == FALSE);

	glBindAttribLocation(programHandle, 0, "a_position");
	glBindAttribLocation(programHandle, 1, "a_segment");
	glBindAttribLocation(programHandle, 2, "a_angles");
	glBindAttribLocation(programHandle, 3, "a_tangents");
	glBindAttribLocation(programHandle, 4, "a_texcoord");
	glBindAttribLocation(programHandle, 5, "a_index");
	//将这些对象链接成一个可执行程序  
	glLinkProgram(programHandle);

	glDeleteShader(vShader);
	glDeleteShader(fShader);
	//查询链接的结果  
	GLint linkStatus;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus);
	if (GL_FALSE == linkStatus)
	{
		cerr << "ERROR : link shader program failed" << endl;
		GLint logLen;
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH,
			&logLen);
		if (logLen > 0)
		{
			char *log = (char *)malloc(logLen);
			GLsizei written;
			glGetProgramInfoLog(programHandle, logLen,
				&written, log);
			cerr << "Program log : " << endl;
			cerr << log << endl;
		}
	}
	else//链接成功，在OpenGL管线中使用渲染程序  
	{
		glUseProgram(programHandle);
	}
}

void initVBO()
{
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	// Create and populate the buffer objects  
	GLuint vboHandles[6];
	glGenBuffers(6, vboHandles);	
	GLuint ptsHandle = vboHandles[0];
	//绑定VBO以供使用  
	glBindBuffer(GL_ARRAY_BUFFER, ptsHandle);
	//加载数据到VBO  
	glBufferData(GL_ARRAY_BUFFER, m_pts.size() * sizeof(vec2), &m_pts[0], GL_DYNAMIC_DRAW);

	GLuint segHandle = vboHandles[1];
	//绑定VBO以供使用  
	glBindBuffer(GL_ARRAY_BUFFER, segHandle);
	//加载数据到VBO  
	glBufferData(GL_ARRAY_BUFFER, m_segment.size() * sizeof(vec2), &m_segment[0], GL_DYNAMIC_DRAW);

	GLuint anglesHandle = vboHandles[2];
	//绑定VBO以供使用  
	glBindBuffer(GL_ARRAY_BUFFER, anglesHandle);
	//加载数据到VBO  
	glBufferData(GL_ARRAY_BUFFER, m_angles.size() * sizeof(vec2), &m_angles[0], GL_DYNAMIC_DRAW);

	GLuint tanHandle = vboHandles[3];
	//绑定VBO以供使用  
	glBindBuffer(GL_ARRAY_BUFFER, tanHandle);
	//加载数据到VBO  
	glBufferData(GL_ARRAY_BUFFER, m_tangents.size() * sizeof(vec4), &m_tangents[0], GL_DYNAMIC_DRAW);

	GLuint texHandle = vboHandles[4];
	//绑定VBO以供使用  
	glBindBuffer(GL_ARRAY_BUFFER, texHandle);
	//加载数据到VBO  
	glBufferData(GL_ARRAY_BUFFER, m_texcoord.size() * sizeof(vec2), &m_texcoord[0], GL_DYNAMIC_DRAW);

	GLuint indexHandle = vboHandles[5];
	//绑定VBO以供使用  
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexHandle);
	//加载数据到VBO  
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_index.size() * sizeof(unsigned short), &m_index[0], GL_DYNAMIC_DRAW);


	glBindVertexArray(vaoHandle);
	glEnableVertexAttribArray(0);//顶点坐标  
	glBindBuffer(GL_ARRAY_BUFFER, ptsHandle);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (GLubyte *)NULL);

	glEnableVertexAttribArray(1);//段坐标
	glBindBuffer(GL_ARRAY_BUFFER, segHandle);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (GLubyte *)NULL);

	glEnableVertexAttribArray(2);//段坐标
	glBindBuffer(GL_ARRAY_BUFFER, anglesHandle);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (GLubyte *)NULL);

	glEnableVertexAttribArray(3);//正切坐标
	glBindBuffer(GL_ARRAY_BUFFER, tanHandle);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (GLubyte *)NULL);

	glEnableVertexAttribArray(4);//纹理坐标
	glBindBuffer(GL_ARRAY_BUFFER, texHandle);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (GLubyte *)NULL);

	glBindVertexArray(0);
}

void init()
{
	//初始化glew扩展库  
	GLenum err = glewInit();
	assert(glGetError() == FALSE);
	if (GLEW_OK != err)
	{
		cout << "Error initializing GLEW: " << glewGetErrorString(err) << endl;
	}

	//开启深度测试
	glDisable(GL_DEPTH_TEST);

	glClearColor(1.0, 1.0, 1.0, 1.0);
	assert(glGetError() == FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//初始化数据
	vector<Point2D> positions;
	positions.push_back(Point2D(-100.0, -100.0));
	positions.push_back(Point2D(200.0, -100.0));
	positions.push_back(Point2D(100.0, 100.0));
	positions.push_back(Point2D(-100.0, 100.0));
	positions.push_back(Point2D(-60.0, 170.0));
	positions.push_back(Point2D(60.0, 0.0));
	m_color = vec4(1.0, 0.0, 0.0, 1.0);

	////生成相应数据
	InitData(positions);
	initVBO();
	assert(glGetError() == FALSE);
	initShader("basic.vert", "basic.frag");
	glUseProgram(programHandle);
	glUniform4f(glGetUniformLocation(programHandle, "u_color"), m_color.x, m_color.y, m_color.z, m_color.w);
	glUniform1f(glGetUniformLocation(programHandle, "u_lineWidth"), m_lineWidth);
	glUniform1f(glGetUniformLocation(programHandle, "u_antialias"), m_antialias);
	glUniform2f(glGetUniformLocation(programHandle, "u_lineCaps"), m_lineCaps.x, m_lineCaps.y);
	glUniform1f(glGetUniformLocation(programHandle, "u_linejoin"), m_lineJoin);
	glUniform1f(glGetUniformLocation(programHandle, "u_miter_limit"), m_miter_limit);
	glUniform1f(glGetUniformLocation(programHandle, "u_length"), m_length);
	glUniform1f(glGetUniformLocation(programHandle, "u_closed"), m_bClosed);
	glUseProgram(0);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	M3DMatrix44f mScaleMatrix,  tmpTransform, mFinalTransform, mTranslationMatrix, mRotationMatrix1, mRotationMatrix2, mRotationMatrix, mModelView;

	m3dTranslationMatrix44(mTranslationMatrix, 0.0f, 0.0f, -5.0f * x);
	m3dRotationMatrix44(mRotationMatrix, -xPos * 1.3, 0.0f, 0.0f, 1.0f);

	m3dScaleMatrix44(mScaleMatrix, xScale, xScale, xScale);
	m3dMatrixMultiply44(mRotationMatrix, mRotationMatrix, mScaleMatrix);
	m3dMatrixMultiply44(mModelView, mTranslationMatrix, mRotationMatrix);
	m3dMatrixMultiply44(mFinalTransform, projMatrix, mModelView);
	glUseProgram(programHandle);
	glUniformMatrix4fv(glGetUniformLocation(programHandle, "u_mvpMatrix"), 1, GL_FALSE, mFinalTransform);
	glBindVertexArray(vaoHandle);
	int nIndices = m_index.size();
	if (0 != nIndices)
	{
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_SHORT, NULL);
	}

	glBindVertexArray(0);

	glutSwapBuffers();
	glutPostRedisplay();
}

void keyboard(int key, int x, int y)
{
	GLfloat stepSize = 0.025f;
	if (key == GLUT_KEY_UP)
	{
		yPos += stepSize;
	}

	if (key == GLUT_KEY_DOWN)
	{
		yPos -= stepSize;
	}

	if (key == GLUT_KEY_LEFT)
	{
		xPos -= stepSize;
	}

	if (key == GLUT_KEY_RIGHT)
	{
		xPos += stepSize;
	}
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
	{
		mouseX = x;
		mouseY = y;
	}
	else if ((button == 3) && (state == GLUT_UP))
	{
		xScale += 0.03;
	}
	else if ((button == 4) && (state == GLUT_DOWN))
	{
		xScale -= 0.03;
	}
	glutPostRedisplay();
}


void motionMouse(int x, int y)
{
	xPos += float(mouseX - x) / 800;
	yPos += float(mouseY - y) / 600;
	mouseX = x;
	mouseY = y;
}

void ChangeSize(int w, int h)
{
	if (0 == h)
	{
		h = 1;
	}

	glViewport(0, 0, w, h);
	SetPerspective(45.0f, float(w) / float(h), 0.01f, 1000.0f);
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Line");
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(ChangeSize);
	glutSpecialFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motionMouse);

	glutMainLoop();
	return 0;
}