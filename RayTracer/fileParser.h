#pragma once

#include <iostream> //input output stream --> a file in the standard library
//#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib> //For exit function
#include <cstdio>
#include <string>
#include <map>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include <algorithm> // in order to use "replace" function for testParsing

#include "structs.h"

using namespace std;

//vectors for the specific structs

extern View view;
extern vector <Sphere> spheres;
extern vector <Light> lights;
extern Background background_info;
extern Output output_name;
extern vector <Ray> rays;

//* FUNCTION DECLARATIONS OF PARCING.CPP *//

//STEP 1 --> reads file and returns it in the form of a string
string read_file(char* file_name);
//STEP 2 --> parse the string
void parse_file_string(string file);
//STEP 3 --> populate the right structs with the parsed string
void populate_structs(char* lines);
