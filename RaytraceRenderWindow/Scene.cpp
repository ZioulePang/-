#include "Scene.h"
#include <limits>
#include "ArcBall.h"
#include "math.h"
Scene::Scene(std::vector<ThreeDModel> *texobjs,RenderParameters *renderp)
{
    objects = texobjs;
    rp = renderp;

    Cartesian3 ambient = Cartesian3(0.5f,0.5f,0.5f);
    Cartesian3 diffuse = Cartesian3(0.5f,0.5f,0.5f);
    Cartesian3 specular = Cartesian3(0.5f,0.5f,0.5f);
    Cartesian3 emissive = Cartesian3(0,0,0);
    Cartesian3 color = Cartesian3(0,0,0);
    float shininess = 1.0f;

    default_mat = new Material(ambient,diffuse,specular,emissive,shininess);
}



Matrix4 Scene::getModelview()
{
    Matrix4 result;
    result.SetIdentity();
    //TODO: Grab all the necessary matrices to build your modelview. Sliders, Arcball, centering.
    //VCS
    Matrix4 view,model,centering;
    view.SetIdentity();
    model.SetIdentity();
    centering.SetIdentity();
    view.SetTranslation(Cartesian3(rp->xTranslate,rp->yTranslate,rp->zTranslate-1));
    //glTranslatef(rp->xTranslate,rp->yTranslate,rp->zTranslate-1);
    //model.SetRotation(rp->rotationMatrix);
    model = rp->rotationMatrix;
    result = view.operator*(model).operator*(centering);
    return result;
}

//updateScene will build the scene like we would do for
//rasterization. Basically go through the model and
//create triangles. We however need to transform things
//to VCS to raytrace

void Scene::updateScene()
{
    triangles.clear();

    //We go through all the objects to construct the scene
    for (int i = 0;i< int(objects->size());i++)
    {
        typedef unsigned int uint;
        ThreeDModel obj = objects->at(uint(i));

        // loop through the faces: note that they may not be triangles, which complicates life
        for (unsigned int face = 0; face < obj.faceVertices.size(); face++)
        { // per face
            // on each face, treat it as a triangle fan starting with the first vertex on the face
            for (unsigned int triangle = 0; triangle < obj.faceVertices[face].size() - 2; triangle++)
            { // per triangle
                // now do a loop over three vertices
                Triangle t;
                for (unsigned int vertex = 0; vertex < 3; vertex++)
                { // per vertex
                    // we always use the face's vertex 0
                    uint faceVertex = 0;
                    // so if it isn't 0, we want to add the triangle base ID
                    if (vertex != 0)
                        faceVertex = triangle + vertex;

                    //this is our vertex before any transformations. (world space)
                    Homogeneous4 v =  Homogeneous4(
                                obj.vertices[obj.faceVertices   [face][faceVertex]].x,
                            obj.vertices[obj.faceVertices   [face][faceVertex]].y,
                            obj.vertices[obj.faceVertices   [face][faceVertex]].z
                            );

                    //This will start working when you write the method above!
                    v = getModelview()*v;// view * model * v;

                    t.verts[vertex] = v;

                    Homogeneous4 n =  Homogeneous4(
                                obj.normals[obj.faceNormals   [face][faceVertex]].x,
                            obj.normals[obj.faceNormals   [face][faceVertex]].y,
                            obj.normals[obj.faceNormals   [face][faceVertex]].z,
                            0.0f);

                    n = getModelview()*n;
                    t.normals[vertex] = n;

                    Cartesian3 tex = Cartesian3(
                                obj.textureCoords[obj.faceTexCoords[face][faceVertex]].x,
                            obj.textureCoords[obj.faceTexCoords[face][faceVertex]].y,
                            0.0f
                            );
                    t.uvs[vertex] = tex;


                    t.colors[vertex] = Cartesian3( 0.7f, 0.7f, 0.7f);

                } // per vertex
                t.validate();
                if(obj.material== nullptr){
                    t.shared_material = default_mat;
                }else{
                    t.shared_material = obj.material;
                }
                triangles.push_back(t);
            } // per triangle
        } // per face
    }//per object
}

Scene::CollisionInfo Scene::closestTriangle(Ray r)
{
    //TODO: method to find the closest triangle!
    Scene::CollisionInfo ci;
    float max = 1000;
    for (auto t:triangles)
    {
       float temp = t.intersect(r);
       if (temp > 0 && temp < max)
       {
           ci.tri = t;
           ci.t = temp;
           max = temp;
       }
    }
    return ci;
}

Cartesian3 Scene::Blinn_Phong(Light l,Triangle t, Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye)
{
    //light direction
    Cartesian3 lightDir = (getModelview().operator*(l.GetPositionCenter()).Vector().operator-(intersection)).unit();
    //Ambient
    //float ambient_intensity = 0.1f;
    Cartesian3 Ambient,Diffuse,Specular,result = Cartesian3(0,0,0);
    if(t.shared_material != NULL)
    {
        Ambient = l.GetColor().modulate(t.shared_material->ambient).Vector();

       //Diffuse
       float diff = std::fmax(normal.dot(lightDir.unit()),0.0f);
       float diffuse_intensity = 0.5f;
       Diffuse = l.GetColor().modulate(t.shared_material->diffuse).operator*(diff*diffuse_intensity).Vector();

       //Specular
       float specular_intensity = 0.5f;
       float specular_refelectivity = 1.0f;
       float specular_highlight = 32.0;
       Cartesian3 V_e = (eye).operator-(intersection);//viewDir
       Cartesian3 V_b = (lightDir.operator+(V_e)).operator/(2).unit();
       float spec = std::pow(V_b.dot(normal.unit()),specular_highlight);
       Specular = l.GetColor().modulate(t.shared_material->specular).operator*(specular_intensity*specular_refelectivity*spec).Vector();

       //output

       //result = Ambient.operator+(shared_material->emissive);
       result = Ambient.operator+(Diffuse).operator+(Specular).operator+(t.shared_material->emissive);

    }
}

Cartesian3 Scene::Shadow(Light l,Triangle t, Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye)
{


    //light direction
    Cartesian3 lightDir = (getModelview() * l.GetPositionCenter().Vector().operator-(intersection)).unit();
    //Ambient
    //float ambient_intensity = 0.1f;
    Cartesian3 Ambient,Diffuse,Specular,result = Cartesian3(0,0,0);
    if(t.shared_material != NULL)
    {
        Ambient = l.GetColor().modulate(t.shared_material->ambient).Vector();

       //Diffuse
       float diff = std::fmax(normal.dot(lightDir.unit()),0.0f);
       float diffuse_intensity = 0.5f;
       Diffuse = l.GetColor().modulate(t.shared_material->diffuse).operator*(diff*diffuse_intensity).Vector();

       //output

       result = Ambient.operator+(t.shared_material->emissive);

    }

    return result;

}
