#ifndef BASIC_H
#define BASIC_H

#include <iostream>
#include <pthread.h>
#include<sstream>
#include <malloc.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>


#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>


struct glProgram_
{
    GLuint m_;
    operator GLuint(){return m_;}
    glProgram_(GLuint i):m_(i){}
    ~glProgram_(){glDeleteProgram(m_);}
};

struct glShader_
{
    GLuint m_;
    operator GLuint(){return m_;}
    glShader_(GLuint i):m_(i){}
    ~glShader_(){glDeleteShader(m_);}
};

struct glBuffer_
{
    GLuint m_;
    operator GLuint(){return m_;}
    glBuffer_(GLuint i):m_(i){}
    ~glBuffer_(){glDeleteBuffers(1,&m_);}
};

bool CompileErrorhandling(GLuint cs)
{
    GLint length, result;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        char *log;
        // get the shader info log
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &length);
        log = (char*)malloc(length);
        glGetShaderInfoLog(cs, length, &result, log);

        // print an error message and the info log
        printf("shaderCompileFromFile(): Unable to compile : %s\n", log);
        free(log);

        glDeleteShader(cs);
        return false;
    }
    fflush(stdout);
    return true;
}

bool LinkErrorhandling(GLuint progHandle)
{
    GLint result;
    glGetProgramiv(progHandle, GL_LINK_STATUS, &result);
    if (!result) {
        fprintf(stderr, "Error in linking compute shader program\n");
        GLchar log[10240];
        GLsizei length;
        glGetProgramInfoLog(progHandle, 10239, &length, log);
        fprintf(stderr, "Linker log:\n%s\n", log);
        return false;
    }
    fflush(stdout);
    return true;
}

struct complex_
{
    complex_():x(0),y(0){}
    complex_(float r):x(r),y(0){}
    float x,y;
};

GLuint Create1DBuffer(complex_ *data,size_t numberOfelements)
{
    GLuint shaderStrorage=0;
    glGenBuffers(1,&shaderStrorage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStrorage);
    glBufferData(GL_SHADER_STORAGE_BUFFER,numberOfelements*sizeof(complex_),data,GL_STATIC_DRAW);

    return shaderStrorage;
}

void Read1DBuffer(GLuint buff,complex_ *data,size_t numberOfElements)
{
    glMemoryBarrier( GL_ALL_BARRIER_BITS);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buff);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,0,numberOfElements*sizeof(complex_),data);
}

//FT library
#include<math.h>
#define PI 3.141593
complex_ operator + (complex_ a,complex_ b)		//addition
{
    complex_ z;
    z.x=a.x+b.x;
    z.y=a.y+b.y;
    return z;
}

complex_ operator * (complex_ a,complex_ b)
{
    complex_ z;
    z.x=a.x*b.x-a.y*b.y;
    z.y=a.x*b.y+a.y*b.x;
    return z;
}


template<class T>
complex_ operator * (complex_ a,T z)
{
    a.x=a.x*z;
    a.y=a.y*z;
    return a;
}

template<class T>
complex_ operator / (complex_ a,T z)
{
    a.x=a.x/z;
    a.y=a.y/z;
    return a;
}

complex_ exp(complex_ a)   //pow(e,?)
{
    complex_ z;
    z.x=pow(exp(1.0),a.x)*cos(a.y);
    z.y=pow(exp(1.0),a.x)*sin(a.y);
    return z;
}


complex_ *fourrier(complex_ *a,int n)
{
    complex_ *b,i;
    i.x=0;
    i.y=1;
    b=(complex_*)malloc(sizeof(complex_)*n);

    int k,j;

    for(k=0;k<n;k++)
    {
        b[k].x=0;
        b[k].y=0;
    }

    for(k=0;k<n;k++)
        for(j=0;j<n;j++)
            b[k]=b[k]+a[j]*exp(i*-1*k*j*2*3.1415926/n);


    return b;
}

complex_ *invfourrier(complex_ *a,int n)
{
    complex_ *b,i;
    b=(complex_ *)malloc(sizeof(complex_)*n);

    i.x=0;
    i.y=1;

    int k,j;

    for(j=0;j<n;j++)
    {
        b[j].x=0;
        b[j].y=0;
    }

    for(k=0;k<n;k++)
        for(j=0;j<n;j++)
            b[k]=b[k]+a[j]*exp(i*k*j*2*3.1415926/n);

    for(k=0;k<n;k++)
    {
        b[k].x=b[k].x/n;
        b[k].y=b[k].y/n;
    }

    return b;
}



#endif // BASIC_H
