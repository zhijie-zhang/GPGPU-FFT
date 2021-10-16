#include "basic.h"

#define BYTE unsigned char
#define MAXSHADERTHREADS 1000  //defines maximum shader threads per group

using namespace std;

Display *display=0;
Window mainw=0;
GLXContext ctx;
void Xinit_GLInit()
{
    char a=(char)0;
    display = XOpenDisplay(&a);  //get the display

    Window root = DefaultRootWindow(display);

    static int attributeList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,GLX_DEPTH_SIZE,16,None };
    XVisualInfo *vi = glXChooseVisual(display, DefaultScreen(display),attributeList);

    mainw = XCreateSimpleWindow(display, root, 100,1,1,1,0,0,0);  //just some window for OGL rendering context
//    XMapWindow(display, mainw);

    ctx = glXCreateContext(display, vi, 0, GL_TRUE);
    glXMakeCurrent (display, mainw, ctx);
}

void shutDown()
{
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, ctx);

    XDestroyWindow(display, mainw);
    XCloseDisplay(display);
}

struct startUp
{
    startUp(){Xinit_GLInit();}
    ~startUp(){shutDown();}
};

//--
//ComputeShaderFFTCreate_Execute
//numberOfelements: number of elements
//--
void ComputeShaderFT1DCreate_Execute(GLuint shaderstorage_in,GLuint shaderstorage_out,size_t numberOfElements,int iForwardT=0 /*1 of IFT*/)
{
    size_t numberOfGroups=numberOfElements/MAXSHADERTHREADS;
    if(numberOfElements%MAXSHADERTHREADS!=0)
        ++numberOfGroups;

    //setting up compute shader
    const char *csSrc[] = {
        "#version 430\n",
        "layout(std430, binding=0) buffer a {vec2 com_in[];};",
        "layout(std430, binding=1) buffer b {vec2 com_ou[];};",
        "layout (local_size_x = 5) in;",
        "\
        vec2 exp1(vec2 v)\
        {\
            vec2 v1;\
            v1.x=pow(exp(1.0),v.x)*cos(v.y);\
            v1.y=pow(exp(1.0),v.x)*sin(v.y);\
            return v1;\
        }\
        vec2  mul1(vec2 a,vec2 b)\
        {\
            vec2 r;\
            r.x=a.x*b.x-a.y*b.y;\
            r.y=a.x*b.y+a.y*b.x;\
            return r;\
        }\
        \
        uniform int nSize;\
        uniform int iForwardT;\
        void main(){\
            vec2 ii;\
            ii.x=0;\
            ii.y=1;\
            if(gl_GlobalInvocationID.x>=nSize) return;\
            \
            float k=gl_GlobalInvocationID.x;\
            com_ou[gl_GlobalInvocationID.x].y=com_ou[gl_GlobalInvocationID.x].x=0;\
            for(int j=0;j<nSize;++j)\
            {\
                vec2 t;t.y=0;t.x=k*j*-2*3.141593/nSize;\
                if(1==iForwardT) t.x=-t.x; /*for Inverse FT*/\
                \
                com_ou[gl_GlobalInvocationID.x]=com_ou[gl_GlobalInvocationID.x]+mul1(com_in[j],exp1(mul1(t,ii)));\
            }\
            if(1==iForwardT) com_ou[gl_GlobalInvocationID.x]=com_ou[gl_GlobalInvocationID.x]/nSize;\
        }"
    };

    stringstream ss;
    ss<<"layout (local_size_x = "<<MAXSHADERTHREADS<<") in;";
    string t=ss.str();
    csSrc[3]=t.c_str();

    glProgram_ progHandle = glCreateProgram();
    glShader_ cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, sizeof(csSrc)/sizeof(char*), csSrc, NULL);

    glCompileShader(cs); if(false==CompileErrorhandling(cs)) return;   /*no need to check in production code*/
    glAttachShader(progHandle, cs);
    glLinkProgram(progHandle);if(false==LinkErrorhandling(progHandle)) return;

    const GLuint ssbos[] = {shaderstorage_in,shaderstorage_out};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 2, ssbos);

    GLint gli0=glGetUniformLocation(progHandle, "nSize");
    GLint gli1=glGetUniformLocation(progHandle, "iForwardT");

    glUseProgram(progHandle);

    glUniform1i(gli0, numberOfElements);
    glUniform1i(gli1, iForwardT);
    cout<<"Compute Shader Group Count:"<<numberOfGroups<<endl;
    glDispatchCompute(numberOfGroups,1, 1);
    glUseProgram(0);
}

template<int N>
void RunFTOnGPU(complex_ (&fin)[N],complex_ (&fout)[N],int iForwardFT=0)
{
    glBuffer_ buff_in=Create1DBuffer(fin,sizeof(fin)/sizeof(complex_));
    glBuffer_ buff_ou=Create1DBuffer(fout,sizeof(fout)/sizeof(complex_));

    ComputeShaderFT1DCreate_Execute(buff_in,buff_ou,sizeof(fin)/sizeof(complex_),iForwardFT);

    Read1DBuffer(buff_ou,fout,sizeof(fout)/sizeof(complex_));
}

template<int N>
void RunFTOnCPU(complex_ (&fin)[N],complex_ (&fout)[N],int iForwardFT=0)
{
    memset(fout,0,sizeof(fout));
    complex_ *f=0;
    if(0==iForwardFT)
        f=fourrier(fin,sizeof(fin)/sizeof(complex_));
    else if(1==iForwardFT)
        f=invfourrier(fin,sizeof(fin)/sizeof(complex_));

    memcpy(fout,f,sizeof(fout));
    free(f);
}


template<int N>
void printData(complex_ (&fin)[N])
{
    size_t i=0;
    for(const complex_ &c:fin)
    {
        cout<<i<<": "<<c.x<<" "<<c.y<<endl;
        ++i;
    }
    cout<<endl;
}

int main()
{
    startUp st;//this required to initialize Opengl comtext using GLX
#define TESTSIZE 5
    complex_ fin[TESTSIZE];
    complex_ fout[TESTSIZE]={};

    for(int i=0;i<TESTSIZE;++i)
    {
        fin[i].x=i;
        fin[i].y=i;
    }

    {
        time_t s=time(0);
        RunFTOnGPU(fin,fout,0);
        time_t e=time(0);
        cout<<e-s<<endl;

        printData(fout);
    }

    //repeat on CPU  for testing
    {
        time_t s=time(0);
        RunFTOnCPU(fin,fout,0);
        time_t e=time(0);
        cout<<e-s<<endl;

        printData(fout);
    }
/*return 0;*/
    //lets do an extended test
#define TESTSIZE1 1234
    complex_ fin1[TESTSIZE1];
    complex_ fin2[TESTSIZE1]={};

    for(int i=0;i<TESTSIZE1;++i)
    {
        fin2[i]=fin1[i].x=i;
        fin2[i]=fin1[i].y=i;
    }

    {
        time_t s=time(0);
        RunFTOnGPU(fin1, fin1,0);//this is crazy
        RunFTOnGPU(fin1, fin1,0);
        RunFTOnGPU(fin1, fin1,0);

        RunFTOnGPU(fin1,fin1, 1);
        RunFTOnGPU(fin1,fin1, 1);
        RunFTOnGPU(fin1,fin1, 1);

        time_t e=time(0);

        printData(fin1);
        cout<<"on GPU:"<<e-s<<endl;
    }
    //do the CPU test to compare
    {
        time_t s=time(0);
        RunFTOnCPU(fin2, fin2,0);//this is crazy
        RunFTOnCPU(fin2, fin2,0);
        RunFTOnCPU(fin2, fin2,0);

        RunFTOnCPU(fin2,fin2, 1);
        RunFTOnCPU(fin2,fin2, 1);
        RunFTOnCPU(fin2,fin2, 1);

        time_t e=time(0);

        printData(fin1);
        cout<<"on CPU:"<<e-s<<endl;
    }

    return 0;
}
