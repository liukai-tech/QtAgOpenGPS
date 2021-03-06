#include "cworldgrid.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QQuickView>
#include <QThread>
#include <assert.h>
#include "aogsettings.h"
#include "glutils.h"

struct SurfaceVertex {
    QVector3D vertex;
    QVector2D textureCoord;
};

CWorldGrid::CWorldGrid()
{

}

CWorldGrid::~CWorldGrid() {
    //get opengl context for main qml view
    //mf->qmlview->openglContext()->currentContext()->makeCurrent
    //fieldBuffer.destroy();
    //gridBuffer.destroy();
}

void CWorldGrid::drawFieldSurface(QOpenGLFunctions *gl, const QMatrix4x4 &mvp, QColor fieldColor)
{
    QSettings settings;


    //We can save a lot of time by keeping this grid buffer on the GPU unless it needs to
    //be altered.
    if (!fieldBufferCurrent) {
        //regenerate the field surface VBO
        QVector<QVector3D> vertices;

        SurfaceVertex field[] = {
            { QVector3D(eastingMin, northingMax, 0.0),
              QVector2D(0,0) },
            { QVector3D(eastingMax, northingMax, 0.0),
              QVector2D(texZoomE, 0) },
            { QVector3D(eastingMin, northingMin, 0.0),
              QVector2D(0,texZoomN) },
            { QVector3D(eastingMax, northingMin, 0.0),
              QVector2D(texZoomE, texZoomN) }
        };

        if (fieldBuffer.isCreated())
            fieldBuffer.destroy();
        fieldBuffer.create();
        fieldBuffer.bind();
        fieldBuffer.allocate(field, sizeof(SurfaceVertex) * 4);
        fieldBuffer.release();

        fieldBufferCurrent = true;
    }

    //bind field surface texture
    texture[Textures::FLOOR]->bind();
    //qDebug() << texture[Textures::FLOOR] -> width();

    glDrawArraysTexture(gl, mvp, GL_TRIANGLE_STRIP, fieldBuffer, GL_FLOAT, 4, true, fieldColor);

    texture[Textures::FLOOR]->release();
    /*
    GLHelperTexture gldraw;
    gldraw.append( { QVector3D(eastingMin, northingMax, 0.0), QVector2D(0,0) } );
    gldraw.append( { QVector3D(eastingMax, northingMax, 0.0), QVector2D(texZoomE, 0) } );
    gldraw.append( { QVector3D(eastingMin, northingMin, 0.0), QVector2D(0,texZoomN) } );
    gldraw.append( { QVector3D(eastingMax, northingMin, 0.0), QVector2D(texZoomE, texZoomN) } );
    gldraw.draw(gl, mvp, Textures::FLOOR, GL_TRIANGLE_STRIP, true, fieldColor);
    */
}

void CWorldGrid::drawWorldGrid(QOpenGLFunctions *gl, const QMatrix4x4 &mvp, double _gridZoom, QColor gridColor)
{
    //draw easting lines and westing lines to produce a grid
    QSettings settings;

    if (_gridZoom != lastGridZoom) { //use epsilon here?
        QVector<QVector3D> vertices;
        lastGridZoom = _gridZoom;

        for (double x = eastingMin; x < eastingMax; x += lastGridZoom)
        {
            //the x lines
            //gl->glVertex3d(x, northingMax, 0.1 );
            vertices.append(QVector3D(x,northingMax, 0.1));

            //gl->glVertex3d(x, northingMin, 0.1);
            vertices.append(QVector3D(x,northingMin, 0.1));
        }

        for (double x = northingMin; x < northingMax; x += lastGridZoom)
        {
            //the z lines
            //gl->glVertex3d(eastingMax, x, 0.1 );
            vertices.append(QVector3D(eastingMax,x, 0.1));

            //gl->glVertex3d(eastingMin, x, 0.1 );
            vertices.append(QVector3D(eastingMin,x, 0.1));
        }

        if (gridBuffer.isCreated())
            gridBuffer.destroy();
        gridBuffer.create();
        gridBuffer.bind();
        gridBuffer.allocate(vertices.data(),vertices.count() * sizeof(QVector3D));
        gridBuffer.release();

        gridBufferCount = vertices.count();

    }
    gl->glLineWidth(1);
    glDrawArraysColor(gl, mvp,
                      GL_LINES, gridColor,
                      gridBuffer,GL_FLOAT,
                      gridBufferCount);
}

void CWorldGrid::createWorldGrid(double northing, double easting) {
    //draw a grid 5 km each direction away from initial fix
     northingMax = northing + 160;
     northingMin = northing - 160;

     eastingMax = easting + 160;
     eastingMin = easting - 160;

     //reconstruct VBOs
     fieldBufferCurrent = false;
     lastGridZoom = -1;
}


void CWorldGrid::checkZoomWorldGrid(double northing, double easting) {
    //make sure the grid extends far enough away as you move along
    //just keep it growing as you continue to move in a direction - forever.

    //hmm, buffers are still redone a lot.

    if ((northingMax - northing) < 1000)
    {
        northingMax = northing + 2000;
        texZoomN = (double)(int)((northingMax - northingMin) / 500.0);
        invalidateBuffers();
    }
    if ((northing - northingMin) < 1000) {
        northingMin = northing - 2000;
        texZoomN = (double)(int)((northingMax - northingMin) / 500.0);
        invalidateBuffers();
    }
    if ((eastingMax - easting) < 1000)
    {
        eastingMax = easting + 2000;
        texZoomE = (double)(int)((eastingMax - eastingMin) / 500.0);
        invalidateBuffers();
    }
    if ((easting - eastingMin) < 1000) {
        eastingMin = easting - 2000;
        texZoomE = (double)(int)((eastingMax - eastingMin) / 500.0);
        invalidateBuffers();
    }
}

void CWorldGrid::destroyGLBuffers() {
    //assume valid OpenGL context
    delete fieldShader;
    fieldShader = 0;

    fieldBuffer.destroy();
    gridBuffer.destroy();
}
