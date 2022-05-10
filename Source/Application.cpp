#include "GDT/Window.h"
#include "GDT/Shader.h"
#include "GDT/Matrix4f.h"
#include "GDT/Vector3f.h"
#include "GDT/Maths.h"
#include "GDT/OpenGL.h"

#include "Sound.h"
#include "AudioFile.h"
#include "CustomAudioSource.h"
#include <fftw3.h>
#include "soloud.h"
#include "soloud_wav.h"

#include <cmath>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>

#define NUM_PARTICLES 100000

std::default_random_engine generator = std::default_random_engine();
std::uniform_real_distribution<float> distribution = std::uniform_real_distribution<float>(0, 1);
std::normal_distribution<float> norm_dist = std::normal_distribution<float>();

void generatePoints(Vector3f min, Vector3f max, std::vector<Vector3f>& points)
{
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        float x = distribution(generator) * 2 - 1;
        float y = distribution(generator) * 2 - 1;
        float z = distribution(generator) * 2 - 1;
        Vector3f p(x, y, z);

        points.push_back(p);
    }

    for (int i = 0; i < (int) (NUM_PARTICLES * 0.8f); i++)
    {
        const Vector3f& p = points[i];

        int d = (int)(distribution(generator) * 3);

        if (d == 0)
        {
            points[i].x = p.x < 0 ? min.x : max.x;
            points[i].y = p.y < 0 ? min.y : max.y;
        }
        if (d == 1)
        {
            points[i].y = p.y < 0 ? min.y : max.y;
            points[i].z = p.z < 0 ? min.z : max.z;
        }
        if (d == 2)
        {
            points[i].z = p.z < 0 ? min.z : max.z;
            points[i].x = p.x < 0 ? min.x : max.x;
        }
    }
}

void generateSphere(std::vector<Vector3f>& points)
{
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        float y = norm_dist(generator);
        float x = norm_dist(generator) * y;
        float z = norm_dist(generator) * y;
        Vector3f p(x, y, z);
        p.normalize();

        points.push_back(p);
    }
}

//void generateSphere(std::vector<Vector3f>& points)
//{
//    for (int i = 0; i < NUM_PARTICLES; i++)
//    {
//        float x = norm_dist(generator);
//        float y = norm_dist(generator);
//        float z = norm_dist(generator);
//        Vector3f p(x, y, z);
//        p.normalize();
//
//        points.push_back(p);
//    }
//}

Vector3f lerp(Vector3f a, Vector3f b, float t)
{
    return a * t + b * (1 - t);
}

class Application : public MouseMoveListener, KeyListener
{
public:
    Application()
    {

    }

    void init()
    {
        //audioFile.load("after_youve_gone.wav");
        //sampleRate = 44100;
        //sampleRate = audioFile.getSampleRate();
        //int bitDepth = audioFile.getBitDepth();
        numSamples = 1000000;
        //numSamples = audioFile.getNumSamplesPerChannel();
        //double lengthInSeconds = audioFile.getLengthInSeconds();

        //int numChannels = audioFile.getNumChannels();
        //bool isMono = audioFile.isMono();
        //bool isStereo = audioFile.isStereo();

        // or, just use this quick shortcut to print a summary to the console
        //audioFile.printSummary();

        gSoloud.init();
        //gWave.load("after_youve_gone.wav");
        
        soundHandle = gSoloud.play(audioSource);

        _window.create("Particle Shapes", 1024, 1024);
        _window.lockCursor(true);

        _window.addMouseMoveListener(this);
        _window.addKeyListener(this);

        try
        {
            _shader.create();
            _shader.addShaderFromFile(VERTEX, "Resources/basic.vert");
            _shader.addShaderFromFile(FRAGMENT, "Resources/basic.frag");
            _shader.build();
            _quadShader.create();
            _quadShader.addShaderFromFile(VERTEX, "Resources/Quad.vert");
            _quadShader.addShaderFromFile(FRAGMENT, "Resources/Texture.frag");
            _quadShader.build();
            _blurShader.create();
            _blurShader.addShaderFromFile(VERTEX, "Resources/Quad.vert");
            _blurShader.addShaderFromFile(FRAGMENT, "Resources/Blur.frag");
            _blurShader.build();
        }
        catch (ShaderLoadingException& e)
        {
            std::cout << e.what() << std::endl;
        }

        glGenTextures(1, &_bufTex);
        glBindTexture(GL_TEXTURE_2D, _bufTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _window.getWidth(), _window.getHeight(), 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenTextures(1, &_buf2Tex);
        glBindTexture(GL_TEXTURE_2D, _buf2Tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _window.getWidth()/2, _window.getHeight()/2, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenTextures(1, &_buf3Tex);
        glBindTexture(GL_TEXTURE_2D, _buf3Tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _window.getWidth() / 2, _window.getHeight() / 2, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glBindTexture(GL_TEXTURE_2D, _bufTex);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _bufTex, 0);

        glGenFramebuffers(1, &_blurFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _blurFbo);
        glBindTexture(GL_TEXTURE_2D, _buf2Tex);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _buf2Tex, 0);
        glBindTexture(GL_TEXTURE_2D, _buf3Tex);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _buf3Tex, 0);
        GLuint* buffers = new GLuint[3]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, buffers);

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        generatePoints(Vector3f(-0.5f, -0.5f, -2), Vector3f(0.5f, 0.5f, 2), pointsA);
        //generatePoints(Vector3f(-2, -1, -1), Vector3f(2, 1, 1), pointsB);
        generateSphere(pointsB);

        points.resize(NUM_PARTICLES * 2);
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            points[i] = pointsA[i];
        }

        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vector3f), points.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
        glEnableVertexAttribArray(0);

        //glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glClearColor(0, 0, 0, 1);

        camPosition.set(0, 0, -5);
    }
#define TWO_PI 6.28318530718
    void update()
    {
        while (!_window.shouldClose())
        {
            SoLoud::time streamTime = gSoloud.getStreamPosition(soundHandle);
            //std::cout << "Stream time: " << streamTime << std::endl;
            time = streamTime;
            
            // Get frequencies
            int channel = 0;

            size_t N = 1000;
            //int sampleTime = max(0, streamTime * sampleRate - (N / 2));
            //std::vector<double> samples(N);
            //for (int i = sampleTime; i < sampleTime + N; i++)
            //{
            //    double currentSample = audioSource.mSamples[i];
            //    samples[i - sampleTime] = currentSample;
            //}

            //fftw_complex* in, * out;
            //fftw_plan p;

            //in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
            //float meanAmp = 0;
            //for (int i = 0; i < N; i++)
            //{
            //    in[i][0] = samples[i];
            //    meanAmp += abs(samples[i]);
            //}
            ////meanAmp /= N;
            //out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
            //p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

            //fftw_execute(p); /* repeat as needed */

            //std::ofstream myfile;
            //myfile.open("outsamples.csv");
            //for (int i = 0; i < N; i++)
            //{
            //    //std::cout << out[i][0] << std::endl;
            //    myfile << out[i][0] << std::endl;
            //}

            //myfile.close();

            //std::cout << "t: " << sampleTime << " amp: " << out[2000][0] << std::endl;

            if (forward)
                camPosition += Vector3f(0, 0, 0.1f);
            if (backward)
                camPosition += Vector3f(0, 0, -0.1f);

            for (int i = 0; i < NUM_PARTICLES; i++)
            {
                points[i] = lerp(pointsA[i], pointsB[i], cosineWave(2.1f, time) * 0.5 + 0.5); //  * out[2000][0] // sin(time) * 0.5 + 0.5
            }
            //std::cout << "Mean amp: " << meanAmp << std::endl;
            //for (int i = NUM_PARTICLES; i < NUM_PARTICLES * 2; i++)
            //{
            //    const Vector3f& p = points[i - NUM_PARTICLES];

            //    Vector3f q(p.x, -1.2 + meanAmp*0.01f, p.z);

            //    points[i] = q;
            //}
            //
            //fftw_destroy_plan(p);
            //fftw_free(in); fftw_free(out);


            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vector3f), points.data(), GL_STATIC_DRAW);

            float _fieldOfView = 60;
            float _aspectRatio = 1;
            float _zNear = 0.1f;
            float _zFar = 100.0f;

            Matrix4f projMatrix;
            float fieldOfViewRadians = _fieldOfView * (Math::PI / 180);
            projMatrix[0] = (1.0f / tan(fieldOfViewRadians / 2)) / _aspectRatio;
            projMatrix[5] = (1.0f / tan(fieldOfViewRadians / 2));
            projMatrix[10] = (_zNear + _zFar) / (_zNear - _zFar);
            projMatrix[11] = -1;
            projMatrix[14] = (2 * _zNear * _zFar) / (_zNear - _zFar);
            projMatrix[15] = 0;

            Matrix4f viewMatrix;
            viewMatrix.rotate(camRotation);
            viewMatrix.translate(camPosition);

            Matrix4f modelMatrix;
            modelMatrix.rotate(time, 0, 1, 0);

            _shader.bind();
            _shader.uniformMatrix4f("projMatrix", projMatrix);
            _shader.uniformMatrix4f("viewMatrix", viewMatrix);
            _shader.uniformMatrix4f("modelMatrix", modelMatrix);

            glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
            glViewport(0, 0, 1024, 1024);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_POINTS, 0, NUM_PARTICLES * 2);



            // Perform horizontal blurring
            _blurShader.bind();
            _blurShader.uniform1i("tex", 0);
            _blurShader.uniform2f("windowSize", 512, 512);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _bufTex);
            
            _blurShader.uniform2f("direction", 1, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, _blurFbo);
            glViewport(0, 0, 512, 512);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Perform vertical blurring
            glBindTexture(GL_TEXTURE_2D, _buf2Tex);
            _blurShader.uniform2f("direction", 0, 1);

            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


            //// Perform horizontal blurring
            //glBindTexture(GL_TEXTURE_2D, _buf3Tex);

            //_blurShader.uniform2f("direction", 1, 0);

            //glDrawBuffer(GL_COLOR_ATTACHMENT0);
            //glClear(GL_COLOR_BUFFER_BIT);
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            //// Perform vertical blurring
            //glBindTexture(GL_TEXTURE_2D, _buf2Tex);
            //_blurShader.uniform2f("direction", 0, 1);

            //glDrawBuffer(GL_COLOR_ATTACHMENT1);
            //glClear(GL_COLOR_BUFFER_BIT);
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);



            // Draw final buffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, 1024, 1024);

            glClear(GL_COLOR_BUFFER_BIT);
            _quadShader.bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _bufTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _buf3Tex);
            _quadShader.uniform1i("tex", 0);
            _quadShader.uniform1i("bloomTex", 1);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            _window.update();
        }
    }

    void onMouseMove(float x, float y) override
    {
        //camRotation.x += y * 0.01f;
        camRotation.y += x * 0.001f;
    }

    void onKeyPressed(int key, int mods) override
    {
        std::cout << key << std::endl;
        if (key == 87)
        {
            forward = true;
        }
        if (key == 83)
        {
            backward = true;
        }
    }

    void onKeyReleased(int key, int mods) override
    {
        if (key == 87)
        {
            forward = false;
        }
        if (key == 83)
        {
            backward = false;
        }
    }

private:
    Window _window;

    ShaderProgram _shader;
    ShaderProgram _quadShader;
    ShaderProgram _blurShader;

    std::vector<Vector3f> points;

    std::vector<Vector3f> pointsA;
    std::vector<Vector3f> pointsB;

    Vector3f camPosition;
    bool forward = false;
    bool backward = false;
    Vector3f camRotation;

    GLuint _vao;
    GLuint _vbo;

    GLuint _fbo;
    GLuint _blurFbo;

    GLuint _bufTex;
    GLuint _buf2Tex;
    GLuint _buf3Tex;

    //AudioFile<double> audioFile;
    float time = 0;
    int numSamples = 1;
    int sampleRate = 1;

    SoLoud::Soloud gSoloud; // SoLoud engine
    SoLoud::Wav gWave;      // One wave file
    CustomAudioSource audioSource;
    int soundHandle;
};

int main()
{
    Application application;
    application.init();
    application.update();
    return 0;
}
