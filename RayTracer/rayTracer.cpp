#include <cstdlib> 
#include <cstdio>
#include <string>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <map>

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include <algorithm> 
#include "structs.h"
#include "ppm.h"
#include "fileParser.h"

using namespace std;



//Returns the ray's closest intersection points for spheres
pair <bool,IntersectedSphere> compute_closest_intersection(Ray ray) {
    pair <bool, IntersectedSphere> inter_pair;
    inter_pair.first = false;

    // solution vector:
    // --> x is t (ie vector scaler to reach intersection point) 
    // --> y is j=the numbered sphere in the forloop
    vector <vec2> solutions;    

    for (int j=0; j < spheres.size(); j++) {
 
        //invert ray for each sphere based on the sphere's scalars.
        vec4 inverted_starting_point = spheres[j].inverseTransform * vec4{ ray.starting_point,1 };
        vec4 inverted_direction = spheres[j].inverseTransform * vec4{ ray.direction,0 };
       
        // First store S (inverted starting point) and c (inverted direction) in NON-homogeneous coordinates
        vec3 S = { inverted_starting_point};
        vec3 c = { inverted_direction};    
      
        // Use the quadratic formula to check for solutions (intersections)
        // If (determinant >= 0) then find the solution(s) representing the intersection pt(s) 
        const float determinant = pow(dot(S, c), 2) - (pow(length(c), 2) * (pow(length(S), 2) - 1));

        if (determinant >= 0.0){
           
            // In this case the ray hits the sphere either once or twice, so find the solution for t
            float t1 = -(dot(S, c) / pow(length(c), 2)) - (sqrt(determinant)/pow(length(c),2));
            float t2 = -(dot(S, c) / pow(length(c), 2)) + (sqrt(determinant) / pow(length(c), 2));
            
            //store smallest positive t in solutions
            if (t1 >= 0 ) {
                solutions.push_back(vec2{ t1,j });
                spheres[j].cannonicalIntersectionPoint = inverted_starting_point + t1 * inverted_direction;
                inter_pair.first = true;
            }
            else if (t1 < 0 && t2 >= 0) {
                solutions.push_back(vec2{ t2,j });
                spheres[j].cannonicalIntersectionPoint = -(inverted_starting_point + t2 * inverted_direction);
                inter_pair.first = true;
            }

        }        
    }

    //If no solutions found then return a null vec4
    if (inter_pair.first!=true) {
        return inter_pair;
    }
    inter_pair.second.order = solutions[0].y;
    inter_pair.second.t = solutions[0].x;
   
    // Find the closest intersection based on the smallest t value
    for (int i = 1; i < solutions.size(); i++) {       
        if (solutions[i].x < inter_pair.second.t) {          
            inter_pair.second.t = solutions[i].x;
            inter_pair.second.order = solutions[i].y;
        }
    }
       
    // Delete all the entries in the vector of "intersections" so that it can be reused  
    solutions.clear();

    //find the normal at that intersection point
    vec4 normal = vec4{ spheres[inter_pair.second.order].cannonicalIntersectionPoint,0 };
    normal = normal * spheres[inter_pair.second.order].inverseTransform;
    
    inter_pair.second.normal = normalize(vec3{normal});

    inter_pair.second.intersection_point = ray.starting_point + inter_pair.second.t * ray.direction;
    //Save the view vector towards the intersection point
    inter_pair.second.view_vector = -normalize(inter_pair.second.intersection_point);

    return inter_pair;
}



// test if a light intersection is shadowed. If not, compute ads
vec3 shadow_ray(Light light, IntersectedSphere original_intersection) {

    vec3 color = { 0,0,0 };

    //Create a shadow ray (intersection pt-light) with starting point being the intersection_point and direction being the normalized dir towards the light*/
    Ray shadow_ray = Ray();
    shadow_ray.direction = light.position-original_intersection.intersection_point;
    shadow_ray.direction = normalize(shadow_ray.direction);
    shadow_ray.starting_point = original_intersection.intersection_point + 0.0001f *shadow_ray.direction;


    // Compute the closest intersection from the original intersection point of the normalized direction 
    pair <bool, IntersectedSphere> closest_intersection = compute_closest_intersection(shadow_ray);
    if (closest_intersection.first) {
        //It does intersect some spheres so check if the intersected sphere is the same as that of the original intersection
        float light_distance = length(light.position - original_intersection.intersection_point);
        float object_distance = length(closest_intersection.second.t * shadow_ray.direction);

        if (object_distance < light_distance) {
            //return black
            return color;
        }
    }


    //diffuse
    color += light.color * spheres[original_intersection.order].kd * spheres[original_intersection.order].color * std::max(0.0f, dot(original_intersection.normal, shadow_ray.direction));
    
    // calculate the reflected vector
    vec3 reflected_vector = reflect(-shadow_ray.direction , original_intersection.normal);

    //specular
    color += light.color * spheres[original_intersection.order].ks * (float)pow(std::max(0.0f, dot(reflected_vector, original_intersection.view_vector )), spheres[original_intersection.order].spec_exp);
 
    return color;
}

vec3 raytrace(Ray ray) {
    
    //Return base case
    if (ray.depth <= 0) {
        //If maxed out on depth then return black
        return { 0,0,0 };
    }
 
    // find closest intersection for all spheres
    pair <bool,IntersectedSphere> closest_intersection = compute_closest_intersection(ray);

    if (!closest_intersection.first) {
        // if there is no intersection then return the background color if it's the OG ray or black if it's a reflected ray
        if (ray.reflected) {
            return { 0,0,0 };
        }
        else {
            return background_info.color;
        }       
    } 
    // From now on only dealing with intersected objects
    /* Initialize local color of object as ambient*/
    vec3 color_local =  background_info.ambient* spheres[closest_intersection.second.order].ka*spheres[closest_intersection.second.order].color;

    /* STEP 2: check for shadows and add them up*/
   
    color_local+= shadow_ray(lights[0], closest_intersection.second);
    for (int i = 1; i < lights.size(); i++) {
        color_local += shadow_ray(lights[i], closest_intersection.second);
    }
   
    // limit number of reflections
    ray.depth--;
    
    // initialize a new ray for the reflected ray
    Ray reflected_ray = Ray();
    reflected_ray.reflected = true;
    reflected_ray.direction = reflect(ray.direction, closest_intersection.second.normal);
    reflected_ray.depth = ray.depth;
    reflected_ray.starting_point = closest_intersection.second.intersection_point + 0.00001f*reflected_ray.direction;
    vec3 color_reflected = raytrace(reflected_ray);
 
    return (color_local + spheres[closest_intersection.second.order].kr * color_reflected);
}


int main(int argc, char *argv[]) {
    parse_file_string(read_file(argv[1]));
  
    // STEP 1.b: update the scale, scaleTranslate and inverseTransform matrices of each sphere. 
    mat4x4 unit_matrix = { 1, 0, 0, 0,
                           0, 1, 0, 0,
                           0, 0, 1, 0,
                           0, 0, 0, 1 };
        
    for (int m = 0; m < spheres.size();m++) {
        spheres[m].translate = translate(unit_matrix, spheres[m].position);
        spheres[m].scaleTranslate = scale(spheres[m].translate, spheres[m].scaler);
        spheres[m].inverseTransform = inverse(spheres[m].scaleTranslate);
    }

    
    // STEP 2: create correct variables
    int Width = view.resolution.x;
    int Height = view.resolution.y;
    const char *fname6 = output_name.name.c_str();
    Pixel* pixels = new Pixel[Width*Height];

    float Uc;
    float Vr;

    // Step 3: populate pixels
    for (int i = 0; i < Height; i++) {
        for (int j = 0; j < Width; j++) {

            // 1.--> Construct a ray, where ray is a unit vector from eye to pixel
            int row_number = i;
            int column_number = j;
            // --> get pixel position
            int width_screen = view.right;
            int height_screen = view.top;
            Uc = -(float)width_screen + (float)width_screen*(2 * (float)column_number / (float)Width);
            Vr = -(float)height_screen + (float)height_screen * (2 * (float)row_number / (float)Height);

            //innitialize a new ray struct object:
            Ray ray = Ray();
            ray.starting_point.x = Uc;
            ray.starting_point.y = Vr;
            ray.starting_point.z = -view.near;
           
            ray.direction.x = Uc;
            ray.direction.y = Vr;
            ray.direction.z = -view.near;

            // --> make it a unit vector
            ray.direction = normalize(ray.direction);
            ray.depth = 3;
            ray.reflected = false;
            // 2.--> ray trace:
            vec3 final_color = raytrace(ray);
            pixels[i*Width + j].r =std::min(final_color.x,1.0f)*255;
            pixels[i*Width + j].g = std::min(final_color.y,1.0f)*255;
            pixels[i*Width + j].b = std::min(final_color.z,1.0f)*255;

        } 
    }

    //for each row
    save_imageP6(Width, Height, fname6, reinterpret_cast<unsigned char*> (pixels));
    return 0;
}
