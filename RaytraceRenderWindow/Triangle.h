#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Homogeneous4.h"
#include "Ray.h"
#include "Light.h"
#include "Material.h"
#include "RGBAImage.h"
#include "Matrix4.h"


class Triangle
{
private:
    bool valid;

public:
    Homogeneous4 verts[3];
    Homogeneous4 normals[3];
    Homogeneous4 colors[3];
    Cartesian3 uvs[3];

    Material *shared_material;

    Triangle();
    void validate();
    bool isValid();
    float intersect(Ray r);

    Cartesian3 baricentric(Cartesian3 o);
    Cartesian3 Blinn_Phong(Light l,Matrix4 m,Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye);
    Cartesian3 Shadow(Light l,Matrix4 m, Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye);
    Cartesian3 Reflect(Cartesian3 eye,Cartesian3 normal);
    Cartesian3 ReFraction(Cartesian3 incident, Cartesian3 normal,float etai);
    float Fresnel(Cartesian3 incident,Cartesian3 normal,float etai);


};

#endif // TRIANGLE_H
