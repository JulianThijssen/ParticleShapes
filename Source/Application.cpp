#include "GDT/Window.h"
#include "GDT/Shader.h"
#include "GDT/Matrix4f.h"
#include "GDT/Vector3f.h"
#include "GDT/Maths.h"
#include "GDT/OpenGL.h"

#include "Camera.h"
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
#define TWO_PI 6.28318530718

std::default_random_engine generator = std::default_random_engine();
std::uniform_real_distribution<float> distribution = std::uniform_real_distribution<float>(0, 1);
std::normal_distribution<float> norm_dist = std::normal_distribution<float>();

void generatePoints(Vector3f min, Vector3f max, std::vector<Vector3f>& points)
{
    points.resize(NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        float x = distribution(generator) * 2 - 1;
        float y = distribution(generator) * 2 - 1;
        float z = distribution(generator) * 2 - 1;

        points[i].set(x, y, z);
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
    points.resize(NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        float y = norm_dist(generator);
        float x = norm_dist(generator) * y;
        float z = norm_dist(generator) * y;

        points[i].set(x, y, z);
        points[i].normalize();
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
        gSoloud.init();
        
        soundHandle = gSoloud.play(audioSource);

        _window.create("Particle Shapes", winWidth, winHeight);
        _window.enableVSync(true);
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

        glGenTextures(1, &_fboTexture);
        glBindTexture(GL_TEXTURE_2D, _fboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _window.getWidth(), _window.getHeight(), 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &_horizontalBlurTex);
        glBindTexture(GL_TEXTURE_2D, _horizontalBlurTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _window.getWidth()/2, _window.getHeight()/2, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &_verticalBlurTex);
        glBindTexture(GL_TEXTURE_2D, _verticalBlurTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _window.getWidth() / 2, _window.getHeight() / 2, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glBindTexture(GL_TEXTURE_2D, _fboTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fboTexture, 0);

        glGenFramebuffers(1, &_blurFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _blurFbo);
        glBindTexture(GL_TEXTURE_2D, _horizontalBlurTex);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _horizontalBlurTex, 0);
        glBindTexture(GL_TEXTURE_2D, _verticalBlurTex);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _verticalBlurTex, 0);
        GLuint* buffers = new GLuint[3]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, buffers);

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        generatePoints(Vector3f(-0.5f, -0.5f, -2), Vector3f(0.5f, 0.5f, 2), pointsA);
        //generatePoints(Vector3f(-2, -1, -1), Vector3f(2, 1, 1), pointsB);
        generateSphere(pointsB);

        points.resize(NUM_PARTICLES);
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

        _camera.pos.set(0, 0, -5);
    }

    void update()
    {
        while (!_window.shouldClose())
        {
            SoLoud::time streamTime = gSoloud.getStreamPosition(soundHandle);
            //std::cout << "Stream time: " << streamTime << std::endl;
            time += 0.016f;

            if (forward)
                _camera.pos += Vector3f(0, 0, 0.1f);
            if (backward)
                _camera.pos += Vector3f(0, 0, -0.1f);

            for (int i = 0; i < NUM_PARTICLES; i++)
            {
                points[i] = lerp(pointsA[i], pointsB[i], cosineWave(0.21f, time) * 0.5 + 0.5); //  * out[2000][0] // sin(time) * 0.5 + 0.5
            }

            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vector3f), points.data(), GL_STATIC_DRAW);

            _camera.loadProjectionMatrix(_projMatrix);
            _camera.loadViewMatrix(_viewMatrix);

            Matrix4f modelMatrix;
            modelMatrix.rotate(time, 0, 1, 0);

            _shader.bind();
            _shader.uniformMatrix4f("projMatrix", _projMatrix);
            _shader.uniformMatrix4f("viewMatrix", _viewMatrix);
            _shader.uniformMatrix4f("modelMatrix", modelMatrix);

            glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
            glViewport(0, 0, _window.getWidth(), _window.getHeight());

            // Draw particles
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

            // Perform horizontal blurring
            _blurShader.bind();
            _blurShader.uniform1i("tex", 0);
            _blurShader.uniform2f("windowSize", 1024, 1024);
            _blurShader.uniform1i("numMipMaps", 6);
            _blurShader.uniform1i("minLod", 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _fboTexture);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            _blurShader.uniform2f("direction", 1, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, _blurFbo);
            glViewport(0, 0, 512, 512);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Perform vertical blurring
            glBindTexture(GL_TEXTURE_2D, _horizontalBlurTex);
            glGenerateMipmap(GL_TEXTURE_2D);
            _blurShader.uniform2f("direction", 0, 1);
            _blurShader.uniform2f("windowSize", 512, 512);
            _blurShader.uniform1i("minLod", 0);

            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Draw final buffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, 1024, 1024);

            glClear(GL_COLOR_BUFFER_BIT);
            _quadShader.bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _fboTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _verticalBlurTex);
            _quadShader.uniform1i("tex", 0);
            _quadShader.uniform1i("bloomTex", 1);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            _window.update();
        }
    }

    void onMouseMove(float x, float y) override
    {
        std::cout << "Mouse X:" << x << std::endl;
        float dx = x - mouseX;
        float dy = y - mouseY;

        mouseX = x;
        mouseY = y;
        _camera.rot.x += dy * 0.05f;
        _camera.rot.y += dx * 0.05f;
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

    int winWidth = 1024;
    int winHeight = 1024;

    ShaderProgram _shader;
    ShaderProgram _quadShader;
    ShaderProgram _blurShader;

    std::vector<Vector3f> points;

    std::vector<Vector3f> pointsA;
    std::vector<Vector3f> pointsB;

    Camera _camera;
    Matrix4f _projMatrix;
    Matrix4f _viewMatrix;

    bool forward = false;
    bool backward = false;

    float mouseX = 0, mouseY = 0;

    GLuint _vao;
    GLuint _vbo;

    GLuint _fbo;
    GLuint _blurFbo;

    GLuint _fboTexture;
    GLuint _horizontalBlurTex;
    GLuint _verticalBlurTex;

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
