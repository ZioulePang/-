//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
////////////////////////////////////////////////////////////////////////


#include <math.h>
#include <random>
#include <QTimer>
// include the header file
#include "RaytraceRenderWidget.h"

#define N_THREADS 16
#define N_LOOPS 100
#define N_BOUNCES 5
#define TERMINATION_FACTOR 0.35f

// constructor
RaytraceRenderWidget::RaytraceRenderWidget
        (   
        // the geometric object to show
        std::vector<ThreeDModel>      *newTexturedObject,
        // the render parameters to use
        RenderParameters    *newRenderParameters,
        // parent widget in visual hierarchy
        QWidget             *parent
        )
    // the : indicates variable instantiation rather than arbitrary code
    // it is considered good style to use it where possible
    : 
    // start by calling inherited constructor with parent widget's pointer
    QOpenGLWidget(parent),
    // then store the pointers that were passed in
    texturedObjects(newTexturedObject),
    renderParameters(newRenderParameters),
    raytraceScene(texturedObjects,renderParameters)
    { // constructor
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        restartRaytrace = false;
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &RaytraceRenderWidget::forceRepaint);
        timer->start(30);
    // leaves nothing to put into the constructor body
    } // constructor    

void RaytraceRenderWidget::forceRepaint(){
    update();
}
// destructor
RaytraceRenderWidget::~RaytraceRenderWidget()
    { // destructor
    // empty (for now)
    // all of our pointers are to data owned by another class
    // so we have no responsibility for destruction
    // and OpenGL cleanup is taken care of by Qt
    } // destructor                                                                 

// called when OpenGL context is set up
void RaytraceRenderWidget::initializeGL()
    { // RaytraceRenderWidget::initializeGL()
	// this should remain empty
    } // RaytraceRenderWidget::initializeGL()

// called every time the widget is resized
void RaytraceRenderWidget::resizeGL(int w, int h)
    { // RaytraceRenderWidget::resizeGL()
    // resize the render image
    frameBuffer.Resize(w, h);
    } // RaytraceRenderWidget::resizeGL()
    
// called every time the widget needs painting
void RaytraceRenderWidget::paintGL()
    { // RaytraceRenderWidget::paintGL()
    // set background colour to white
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // and display the image
    glDrawPixels(frameBuffer.width, frameBuffer.height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer.block);
    } // RaytraceRenderWidget::paintGL()


    // routine that generates the image
void RaytraceRenderWidget::Raytrace()
{ // RaytraceRenderWidget::Raytrace()

    restartRaytrace = true;
    if(raytracingThread.joinable())
        raytracingThread.join();
    restartRaytrace = false;

    //To make our lifes easier, lets calculate things on VCS.
    //So we need to process our scene to get a triangle soup in VCS.
    //IMPORTANT: You still need to complete the method that gets the modelview matrix in the scene class!
    raytraceScene.updateScene();

    //clear frame buffer before we start
    frameBuffer.clear(RGBAValue(0.0f, 0.0f, 0.0f,1.0f));

    raytracingThread= std::thread(&RaytraceRenderWidget::RaytraceThread,this);
    raytracingThread.detach();

} // RaytraceRenderWidget::Raytrace()
    

void RaytraceRenderWidget::RaytraceThread()
{
    int loops = renderParameters->monteCarloEnabled? N_LOOPS:1;
    std::cout << "I Will do " << loops << " loops" << std::endl;
    float aspectRatio = float(frameBuffer.width)/float(frameBuffer.height);
    double b = 45 * (3.1415926/180.0);
    double fov = std::tan(b);

    //for(auto l:raytraceScene.rp->lights)
   // {

    //Cartesian3 lightTransfer = Cartesian3(0,0,0);
    //lightTransfer = (raytraceScene.getModelview()*l->GetPositionCenter()).Point();

    // l->SetPositionCenter(lightTransfer);
    //}
    Cartesian3 eye = Cartesian3(0,0,0);
    //Each pixel in parallel using openMP.
    for(int loop = 0; loop < loops; loop++){
        #pragma omp parallel for schedule(dynamic)
        for(int j = 0; j < frameBuffer.height; j++){
            for(int i = 0; i < frameBuffer.width; i++){

                //TODO: YOUR CODE GOES HEREA

                float pixel_x = (float(i) + 0.5f) / frameBuffer.width;
                float pixel_y = (float(j) + 0.5f) / frameBuffer.height;

                float x = (2 * pixel_x - 1) * aspectRatio;
                float y = (1 - 2 * pixel_y);

                Cartesian3 dir = Cartesian3(x,-y,-1).unit();

                Ray r = Ray(eye,dir,Ray::primary);
                Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);

                Cartesian3 o = eye.operator+(dir.operator*(c.t+std::numeric_limits<float>::epsilon() * 300));

                Cartesian3 bc = c.tri.baricentric(o);

                Cartesian3 w = (c.tri.verts[1] - c.tri.verts[0]).Vector();
                Cartesian3 u = (c.tri.verts[2] - c.tri.verts[0]).Vector();
                Cartesian3 normal = (w.cross(u)).unit();

                Cartesian3 newIntersect = o + normal * std::numeric_limits<float>::epsilon() * 500;
                Cartesian3 newBc = c.tri.baricentric(newIntersect);

                Homogeneous4 color; // calculate your raytraced color here.

                //Blinn_Phong
                Cartesian3 blinn_phong = Cartesian3(0,0,0);


                if(c.t>0)
                {
                    if(raytraceScene.rp->interpolationRendering)
                    {
                        //Interepolation
                        blinn_phong.x = std::abs(bc.x);
                        blinn_phong.y = std::abs(bc.y);
                        blinn_phong.z = std::abs(bc.z);
                    }
                    else if(raytraceScene.rp->phongEnabled)
                    {
                        if(raytraceScene.rp->shadowsEnabled)
                            {
                                for(auto l:raytraceScene.rp->lights)
                                {
                                    Matrix4 modelview = raytraceScene.getModelview();
                                    Cartesian3 shadowDir = ((raytraceScene.getModelview().operator*(l->GetPositionCenter())).Vector().operator-(o)).unit();
                                    Ray shadowRay = Ray(newIntersect,shadowDir,Ray::primary);
                                    Scene::CollisionInfo hit =  raytraceScene.closestTriangle(shadowRay);
                                    if(hit.tri.shared_material!=NULL && hit.tri.shared_material->isLight())
                                    {
                                        blinn_phong = c.tri.Blinn_Phong(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                    }else
                                    {
                                        blinn_phong = c.tri.Shadow(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                    }
                                }
                             }
                    }
                    else if(raytraceScene.rp->reflectionEnabled)
                    {
                        if(c.tri.shared_material != NULL && c.tri.shared_material->reflectivity != 0)
                        {
                            blinn_phong = caculateReflection(r,newIntersect,newBc,10);
                        }else
                        {
                            for(auto l:raytraceScene.rp->lights)
                            {
                                Matrix4 modelview = raytraceScene.getModelview();
                                Cartesian3 shadowDir = (raytraceScene.getModelview().operator*(l->GetPositionCenter()).Vector().operator-(o)).unit();
                                Ray shadowRay = Ray(newIntersect,shadowDir,Ray::primary);
                                Scene::CollisionInfo hit =  raytraceScene.closestTriangle(shadowRay);
                                if(hit.tri.shared_material!=NULL && hit.tri.shared_material->isLight())
                                {
                                    blinn_phong = c.tri.Blinn_Phong(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                }
                                else
                                {
                                    blinn_phong = c.tri.Shadow(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                }
                            }
                        }
                    }
                    else if (raytraceScene.rp->refractionEnabled)
                    {
                        if(c.tri.shared_material != NULL && c.tri.shared_material->transparency != 0)
                        {
                            blinn_phong = caculateReflaction(r,newIntersect,49);
                        }
                        else if(c.tri.shared_material != NULL && c.tri.shared_material->reflectivity != 0)
                        {
                             blinn_phong = caculateReflection(r,newIntersect,newBc,10) + blinn_phong;
                        }
                        else
                        {
                            for(auto l:raytraceScene.rp->lights)
                            {
                                Matrix4 modelview = raytraceScene.getModelview();
                                Cartesian3 shadowDir = (raytraceScene.getModelview().operator*(l->GetPositionCenter()).Vector().operator-(o)).unit();
                                Ray shadowRay = Ray(newIntersect,shadowDir,Ray::primary);
                                Scene::CollisionInfo hit =  raytraceScene.closestTriangle(shadowRay);
                                if(hit.tri.shared_material!=NULL && hit.tri.shared_material->isLight())
                                {
                                    blinn_phong = c.tri.Blinn_Phong(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                }
                                else
                                {
                                    blinn_phong = c.tri.Shadow(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                }
                            }
                        }

                    }
                    else if(raytraceScene.rp->fresnelRendering)
                    {
                        if(c.tri.shared_material!=NULL &&c.tri.shared_material->transparency!=0&&c.tri.shared_material->reflectivity!=0)
                        {
                            blinn_phong = caculateFresnel(r,newIntersect,newBc,20);
                        }else
                        {
                            for(auto l:raytraceScene.rp->lights)
                            {
                                Matrix4 modelview = raytraceScene.getModelview();
                                Cartesian3 shadowDir = (raytraceScene.getModelview().operator*(l->GetPositionCenter()).Vector().operator-(o)).unit();
                                Ray shadowRay = Ray(newIntersect,shadowDir,Ray::primary);
                                Scene::CollisionInfo hit =  raytraceScene.closestTriangle(shadowRay);
                                if(hit.tri.shared_material!=NULL && hit.tri.shared_material->isLight())
                                {
                                    blinn_phong = c.tri.Blinn_Phong(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                }
                                else
                                {
                                    blinn_phong = c.tri.Shadow(*l,modelview,newIntersect,newBc,eye)+blinn_phong;
                                }
                            }
                        }
                    }else
                    {
                        blinn_phong.x =1;
                        blinn_phong.y =1;
                        blinn_phong.z =1;
                    }
                    color.x = blinn_phong.x;
                    color.y = blinn_phong.y;
                    color.z = blinn_phong.z;
                }
                //Gamma correction....
                float gamma = 2.2f;
                //We already calculate everything in float, so we just do gamma correction before putting it integer format.
                color.x = pow(color.x,1/gamma)/float(loop+1);
                color.y = pow(color.y,1/gamma)/float(loop+1);
                color.z = pow(color.z,1/gamma)/float(loop+1);
                frameBuffer[j][i] = ((loop)/float(loop+1))*frameBuffer[j][i]+  RGBAValue(color.x*255.0f,color.y*255.0f,color.z*255.0f,255.0f);
                frameBuffer[j][i].alpha = 255;

                }
            }
        std::cout << " Done a loop!" << std::endl;
        if(restartRaytrace){
            return;
        }
    }
    std::cout << "Done!" << std::endl;
}

Cartesian3 RaytraceRenderWidget::caculateReflection(Ray r,Cartesian3 intersection,Cartesian3 normal, int depth)
{
    Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);
    Cartesian3 eye = Cartesian3(0,0,0);
    Cartesian3 blinn_phong;
    if(depth==0) return Cartesian3(0,0,0);
    for(auto l:raytraceScene.rp->lights)
    {
        if(c.tri.shared_material != NULL && c.tri.shared_material->reflectivity !=0)
        {
            Matrix4 modelview = raytraceScene.getModelview();
            Cartesian3 incidentPoint = intersection;
            Cartesian3 emergent = c.tri.Reflect(incidentPoint,normal);
            Ray newR = Ray(incidentPoint,emergent.unit(),Ray::secondary);
            Scene::CollisionInfo newHit =  raytraceScene.closestTriangle(newR);
            Cartesian3 inter = incidentPoint + (newHit.t-0.04f) * emergent.unit();
            Cartesian3 normal2 = newHit.tri.baricentric(inter);
            if(newHit.tri.shared_material != NULL)
            {
                if(newHit.tri.shared_material->reflectivity !=0)
                {
                    return caculateReflection(newR,inter,normal2,depth-1);
                }else
                {
                     return  blinn_phong = (newHit.tri.Blinn_Phong(*l,modelview,incidentPoint,newHit.tri.baricentric(incidentPoint+newHit.t*emergent.unit()),eye).operator*(c.tri.shared_material->reflectivity));
                }
            }
        }
    }
}


Cartesian3 RaytraceRenderWidget::caculateReflaction(Ray r,Cartesian3 intersection, int depth)
{
    Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);
    Cartesian3 eye = Cartesian3(0,0,0);
    Cartesian3 blinn_phong;
    if(depth==0)
    {
        return  Cartesian3(0,0,0);
    }
    if(c.tri.shared_material != NULL)
    {
        if(c.tri.shared_material->transparency !=0)
        {
            Cartesian3 normal = c.tri.baricentric(intersection).unit();
            Cartesian3 emergent = c.tri.ReFraction(intersection,normal,c.tri.shared_material->indexOfRefraction);
            Ray newR = Ray(intersection,emergent.unit(),Ray::primary);
            Scene::CollisionInfo newHit =  raytraceScene.closestTriangle(newR);
            Cartesian3 inter = intersection + (newHit.t+std::numeric_limits<float>::epsilon() * 1000) * emergent.unit();
            if(newHit.tri.shared_material != NULL && newHit.tri.shared_material->transparency==0)
            {
                Matrix4 modelview = raytraceScene.getModelview();
                for(auto l:raytraceScene.rp->lights)
                {
                    Cartesian3 b = newHit.tri.baricentric(inter);
                    blinn_phong = newHit.tri.Blinn_Phong(*l,modelview,inter,b,eye);
                }
                return  blinn_phong;
            }else if (newHit.tri.shared_material != NULL && newHit.tri.shared_material->transparency!=0)
            {
                return caculateReflaction(newR,inter,depth-1);
            }
        }
    }
}

Cartesian3 RaytraceRenderWidget::caculateReflection_Frensel(Ray r,Cartesian3 intersection,Cartesian3 normal, int depth)
{
    Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);
    Cartesian3 eye = Cartesian3(0,0,0);
    Cartesian3 blinn_phong;
    if(depth==0) return Cartesian3(0,0,0);
    for(auto l:raytraceScene.rp->lights)
    {
        if(c.tri.shared_material != NULL && c.tri.shared_material->reflectivity !=0)
        {
            Matrix4 modelview = raytraceScene.getModelview();
            Cartesian3 incidentPoint = intersection;
            Cartesian3 emergent = c.tri.Reflect(incidentPoint,normal);
            Ray newR = Ray(incidentPoint,emergent.unit(),Ray::secondary);
            Scene::CollisionInfo newHit =  raytraceScene.closestTriangle(newR);
            Cartesian3 inter = incidentPoint + (newHit.t-0.04f) * emergent.unit();
            Cartesian3 normal2 = newHit.tri.baricentric(inter);
            if(newHit.tri.shared_material != NULL)
            {
                if(newHit.tri.shared_material->reflectivity !=0)
                {
                    return caculateReflection_Frensel(newR,inter,normal2,depth-1);
                }else
                {
                     return  blinn_phong = (newHit.tri.Blinn_Phong(*l,modelview,incidentPoint,newHit.tri.baricentric(incidentPoint+newHit.t*emergent.unit()),eye).operator*(c.tri.shared_material->reflectivity));
                }
            }
        }
    }
}

Cartesian3 RaytraceRenderWidget::caculateReflaction_Frensel(Ray r,Cartesian3 intersection, int depth)
{
    Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);
    Cartesian3 eye = Cartesian3(0,0,0);
    Cartesian3 blinn_phong;
    if(depth==0)
    {
        return  Cartesian3(0,0,0);
    }
    if(c.tri.shared_material != NULL)
    {
        if(c.tri.shared_material->transparency !=0)
        {
            Cartesian3 normal = c.tri.baricentric(intersection).unit();
            Cartesian3 emergent = c.tri.ReFraction(intersection,normal,c.tri.shared_material->indexOfRefraction);
            Ray newR = Ray(intersection,emergent.unit(),Ray::primary);
            Scene::CollisionInfo newHit =  raytraceScene.closestTriangle(newR);
            Cartesian3 inter = intersection + (newHit.t+std::numeric_limits<float>::epsilon() * 1000) * emergent.unit();
            if(newHit.tri.shared_material != NULL && (newHit.tri.shared_material->isLight()))
            {
                Matrix4 modelview = raytraceScene.getModelview();
                for(auto l:raytraceScene.rp->lights)
                {
                    Cartesian3 b = newHit.tri.baricentric(inter);
                    blinn_phong = newHit.tri.Blinn_Phong(*l,modelview,inter,b,eye);
                }
                return  blinn_phong;
            }else if (newHit.tri.shared_material != NULL && (!newHit.tri.shared_material->isLight()))
            {
                return caculateReflaction(newR,inter,depth-1);
            }
        }
    }
}


Cartesian3 RaytraceRenderWidget::caculateFresnel(Ray r,Cartesian3 intersection,Cartesian3 normal, int depth)
{
    Cartesian3 result,refractColor,reflectColor;
    Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);
    if(c.tri.shared_material != NULL && c.tri.shared_material->transparency != 0)
    {
        int temp_depth = depth;
        refractColor =  c.tri.shared_material->transparency * caculateReflaction_Frensel(r,intersection,temp_depth-1);
    }
    if(c.tri.shared_material != NULL && c.tri.shared_material->reflectivity != 0)
    {
        int temp_depth = depth;
        reflectColor = c.tri.shared_material->reflectivity * caculateReflection_Frensel(r,intersection,normal,temp_depth-1);
    }

    float kr = c.tri.Fresnel(intersection,normal,c.tri.shared_material->indexOfRefraction);

    result = reflectColor * kr + refractColor * (1-kr) ;
    return  result;

}
Cartesian3 RaytraceRenderWidget::caculateShadowReflaction(Ray r,Cartesian3 intersection, int depth)
{
    Scene::CollisionInfo c =  raytraceScene.closestTriangle(r);
    Cartesian3 eye = Cartesian3(0,0,0);
    Cartesian3 blinn_phong;
    if(depth==0)
    {
        return  Cartesian3(0,0,0);
    }
    if(c.tri.shared_material != NULL)
    {
        if(c.tri.shared_material->transparency !=0)
        {
            Cartesian3 normal = c.tri.baricentric(intersection).unit();
            Cartesian3 emergent = c.tri.ReFraction(intersection,normal,c.tri.shared_material->indexOfRefraction);
            Ray newR = Ray(intersection,emergent.unit(),Ray::primary);
            Scene::CollisionInfo newHit =  raytraceScene.closestTriangle(newR);
            Cartesian3 inter = intersection + (newHit.t+std::numeric_limits<float>::epsilon() * 1000) * emergent.unit();
            if(newHit.tri.shared_material != NULL && (newHit.tri.shared_material->isLight()))
            {
                Matrix4 modelview = raytraceScene.getModelview();
                for(auto l:raytraceScene.rp->lights)
                {
                    Cartesian3 b = newHit.tri.baricentric(inter);
                    blinn_phong = newHit.tri.Blinn_Phong(*l,modelview,inter,b,eye);
                }
                return  blinn_phong;
            }else if (newHit.tri.shared_material != NULL && (!newHit.tri.shared_material->isLight()))
            {
                return caculateReflaction(newR,inter,depth-1);
            }
        }
    }
}

// mouse-handling
void RaytraceRenderWidget::mousePressEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mousePressEvent()
    // store the button for future reference
    int whichButton = int(event->button());
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width   
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0f * event->x() - size) / size;
    float y = (size - 2.0f * event->y() ) / size;

    
    // and we want to force mouse buttons to allow shift-click to be the same as right-click
    unsigned int modifiers = event->modifiers();
    
    // shift-click (any) counts as right click
    if (modifiers & Qt::ShiftModifier)
        whichButton = Qt::RightButton;
    
    // send signal to the controller for detailed processing
    emit BeginScaledDrag(whichButton, x,y);
    } // RaytraceRenderWidget::mousePressEvent()
    
void RaytraceRenderWidget::mouseMoveEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mouseMoveEvent()
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width   
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0f * event->x() - size) / size;
    float y = (size - 2.0f * event->y() ) / size;
    
    // send signal to the controller for detailed processing
    emit ContinueScaledDrag(x,y);
    } // RaytraceRenderWidget::mouseMoveEvent()
    
void RaytraceRenderWidget::mouseReleaseEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mouseReleaseEvent()
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width   
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0f * event->x() - size) / size;
    float y = (size - 2.0f * event->y() ) / size;
    
    // send signal to the controller for detailed processing
    emit EndScaledDrag(x,y);
    } // RaytraceRenderWidget::mouseReleaseEvent()
