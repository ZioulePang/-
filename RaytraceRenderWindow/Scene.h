//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
//
//  September, 2022
//
//  ------------------------
//  Scene.h
//  ------------------------
//
//  Contains a definition of a scene, with triangles and transformations.
//
///////////////////////////////////////////////////

#ifndef SCENE_H
#define SCENE_H

#include "Homogeneous4.h"
#include "ThreeDModel.h"
#include <vector>
#include "Ray.h"
#include "Triangle.h"
#include "Material.h"
#include "Light.h"

class Scene
{
public:

    struct CollisionInfo{
        Triangle tri;
        float t;
    };

    std::vector<ThreeDModel>* objects;
    RenderParameters* rp;
    Material *default_mat;

    std::vector<Triangle> triangles;

    Scene(std::vector<ThreeDModel> *texobjs,RenderParameters *renderp);
    void updateScene();
    CollisionInfo closestTriangle(Ray r);
    CollisionInfo ImpuseReflect(Scene::CollisionInfo c,Ray r,Cartesian3 incident,Cartesian3 normal);
    Matrix4 getModelview();
    Cartesian3 Blinn_Phong(Light l,Triangle co,Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye);
    Cartesian3 Shadow(Light l,Triangle t, Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye);
};

#endif // SCENE_H
