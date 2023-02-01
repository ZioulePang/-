#include "Triangle.h"
#include <math.h>
#include "Scene.h"
#include <algorithm>

Triangle::Triangle()
{
    valid = false;
    shared_material= nullptr;
}

float Triangle::intersect(Ray r)
{
    //TODO: Calculate intersection between a ray and a triangle!
    Cartesian3 N_1 = verts[1].Point().operator-(verts[0].Point());//ab
    Cartesian3 N_2 = verts[2].Point().operator-(verts[0].Point());//ac

    Cartesian3 N = N_1.cross(N_2);//normal
    //Caculate intersection point
    float t = (verts[0].Vector().operator-(r.origin)).dot(N) / r.direction.dot(N);

    Cartesian3 p = r.origin.operator+(r.direction.operator*(t));
    if(t<0) return 0;
    if(r.direction.dot(N)==0) return 0;

    Cartesian3 c_1 = (p.operator-(verts[0].Point())).cross(verts[1].Point().operator-(verts[0].Point()));//APxAB
    Cartesian3 c_2 = (p.operator-(verts[1].Point())).cross(verts[2].Point().operator-(verts[1].Point()));//BPxBC
    Cartesian3 c_3 = (p.operator-(verts[2].Point())).cross(verts[0].Point().operator-(verts[2].Point()));//CPxCA

    if((c_1.dot(c_2)>0 && c_2.dot(c_3)>0 && c_3.dot(c_1)>0))
    {
        //p is in the triangle
        //std::cout << t << std::endl;
        return t;
    }

    return 0; // just to compile warning free :)
}

void Triangle::validate(){
    valid = true;
}

bool Triangle::isValid(){
    return valid;
}

Cartesian3 Triangle::baricentric(Cartesian3 o)
{
   //TODO: Implement this! Input is the intersection between the ray and the triangle.

   Cartesian3 ab = verts[1].Point().operator-(verts[0].Point());
   Cartesian3 ac = verts[2].Point().operator-(verts[0].Point());
   Cartesian3 bc = verts[2].Point().operator-(verts[1].Point());
   Cartesian3 ca = verts[0].Point().operator-(verts[2].Point());


   Cartesian3 bo = o.operator-(verts[1].Point());
   Cartesian3 co = o.operator-(verts[2].Point());
   //Cartesian3 ao = o.operator-(verts[0].Point());

   Cartesian3 n = ab.cross(ac);
   Cartesian3 na = bc.cross(bo);
   Cartesian3 nb = ca.cross(co);
   //Cartesian3 nc = ab.cross(ao);

   float normal = (n.length()*n.length());
   float alpha = n.dot(na) / normal;
   float beta = n.dot(nb) / normal;
   float gamma = 1- alpha - beta;

   //Smooth
   Cartesian3 baricentric = ((normals[0].Vector().operator*(alpha)).operator+(normals[1].Vector().operator*(beta)))
           .operator+(normals[2].Vector().operator*(gamma));

   baricentric.x = baricentric.x;
   baricentric.y = baricentric.y;
   baricentric.z = baricentric.z;

   return baricentric.unit();
}


Cartesian3 Triangle::Blinn_Phong(Light l,Matrix4 m, Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye)
{


    //light direction
    Cartesian3 lightDir = (m* l.GetPositionCenter().Vector().operator-(intersection)).unit();
    //Ambient
    //float ambient_intensity = 0.1f;
    Cartesian3 Ambient,Diffuse,Specular,result = Cartesian3(0,0,0);
    if(shared_material != NULL)
    {
        Ambient = l.GetColor().modulate(shared_material->ambient).Vector();

       //Diffuse
       float diff = std::fmax(normal.dot(lightDir.unit()),0.0f);
       float diffuse_intensity = 0.5f;
       Diffuse = l.GetColor().modulate(shared_material->diffuse).operator*(diff*diffuse_intensity).Vector();

       //Specular
       float specular_intensity = 0.5f;
       float specular_refelectivity = 1.0f;
       float specular_highlight = 32.0;
       Cartesian3 V_e = (eye).operator-(intersection);//viewDir
       Cartesian3 V_b = (lightDir.operator+(V_e)).operator/(2).unit();
       float spec = std::pow(V_b.dot(normal.unit()),specular_highlight);
       Specular = l.GetColor().modulate(shared_material->specular).operator*(specular_intensity*specular_refelectivity*spec).Vector();

       //output

       result = Ambient.operator+(Diffuse).operator+(Specular).operator+(shared_material->emissive);

    }



    return result;

}

Cartesian3 Triangle::Shadow(Light l,Matrix4 m, Cartesian3 intersection,Cartesian3 normal,Cartesian3 eye)
{


    //light direction
    Cartesian3 lightDir = (m * l.GetPositionCenter().Vector().operator-(intersection)).unit();
    //Ambient
    //float ambient_intensity = 0.1f;
    Cartesian3 Ambient,Diffuse,Specular,result = Cartesian3(0,0,0);
    if(shared_material != NULL)
    {
        Ambient = l.GetColor().modulate(shared_material->ambient).Vector();

       //Diffuse
       float diff = std::fmax(normal.dot(lightDir.unit()),0.0f);
       float diffuse_intensity = 0.5f;
       Diffuse = l.GetColor().modulate(shared_material->diffuse).operator*(diff*diffuse_intensity).Vector();

       //output

       result = Ambient.operator+(shared_material->emissive);

    }

    return result;

}

Cartesian3 Triangle::Reflect(Cartesian3 incident,Cartesian3 normal)
{
    Cartesian3 N = -2 * incident.dot(normal) * normal;
    Cartesian3 reflect = (N + incident).unit();
    return  reflect;
}

Cartesian3 Triangle::ReFraction(Cartesian3 incident, Cartesian3 normal,float etai)
{
    Cartesian3 emergent = Cartesian3(0,0,0);
    float cosi = incident.unit().dot(normal.unit());
    float etaj = 1;
    Cartesian3 N  = normal;
    if(cosi<0)
    {
        cosi = -cosi;
    }else
    {
        std::swap(etai,etaj);
        N = -1*N;
    }
    float eta = etaj/etai;

    Cartesian3 dir = incident.unit();
    float temp = 1 - ( pow(etaj,2) * (1-pow(dir.dot(N),2))) / pow(etai,2);
    emergent = (etaj*(dir - N*(dir.dot(N))))/etai - N*sqrt(temp);


    return  emergent;
}


float Triangle::Fresnel(Cartesian3 incident,Cartesian3 normal,float etai)
{
    float cosi = incident.unit().dot(normal.unit());
    float etaj = 1;
    if(cosi>0) std::swap(etai,etaj);
    float sint = etaj/etai * sqrtf(std::max(0.0f,1-cosi*cosi));
    if(sint >=1)
    {
        return  1.0f;
    }else
    {
        float cost = sqrtf(std::max(0.0f,1-sint*sint));
        cosi = fabsf(cosi);
        float Rs = ((etai * cosi) - (etaj * cost))/ ((etai*cosi)+(etaj*cost));
        float Rp = ((etaj * cosi) - (etai * cost))/((etaj*cosi) + (etai * cost));

        return(Rs * Rs + Rp * Rp)/2.0f;
    }
}
