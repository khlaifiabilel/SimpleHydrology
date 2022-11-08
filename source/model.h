/*
================================================================================
                            Rendering Parameters
================================================================================
*/

const int WIDTH = 1200;
const int HEIGHT = 800;

bool paused = true;
bool viewmap = false;

//Coloring
float steepness = 0.8;
glm::vec3 flatColor = glm::vec3(0.40, 0.60, 0.25);
glm::vec3 waterColor = glm::vec3(0.17, 0.40, 0.44);
glm::vec3 steepColor = glm::vec3(0.7);

//Lighting and Shading
glm::vec3 skyCol = glm::vec4(0.64, 0.75, 0.9, 1.0f);
glm::vec3 lightPos = glm::vec3(-100.0f, 100.0f, -150.0f);
glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 0.9f);
float lightStrength = 1.4;

//Matrix for Making Stuff Face Towards Light (Trees)
float rot = -1.0f * acos(glm::dot(glm::vec3(1, 0, 0), glm::normalize(glm::vec3(lightPos.x, 0, lightPos.z))));
glm::mat4 faceLight = glm::rotate(glm::mat4(1.0), rot , glm::vec3(0.0, 1.0, 0.0));

//Depth Map Rendering
glm::mat4 dp = glm::ortho<float>(-300, 300, -300, 300, 0, 800);
glm::mat4 dv = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));
glm::mat4 bias = glm::mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);
glm::mat4 dvp = dp*dv;
glm::mat4 dbvp = bias*dvp;

//Vertex Pool and Surface Meshing
uint* section = NULL;

void indexmap(Vertexpool<Vertex>& vertexpool, World& world){

  vertexpool.indices.clear();

  for(int i = 0; i < world.dim.x-1; i++){
  for(int j = 0; j < world.dim.y-1; j++){

    vertexpool.indices.push_back(math::cflatten(i, j, world.dim));
    vertexpool.indices.push_back(math::cflatten(i, j+1, world.dim));
    vertexpool.indices.push_back(math::cflatten(i+1, j, world.dim));

    vertexpool.indices.push_back(math::cflatten(i+1, j, world.dim));
    vertexpool.indices.push_back(math::cflatten(i, j+1, world.dim));
    vertexpool.indices.push_back(math::cflatten(i+1, j+1, world.dim));

  }}

  vertexpool.resize(section, vertexpool.indices.size());
  vertexpool.index();
  vertexpool.update();

}

void updatemap(Vertexpool<Vertex>& vertexpool, World& world){

  for(int i = 0; i < world.dim.x; i++)
  for(int j = 0; j < world.dim.y; j++){

    int ind = math::cflatten(i, j, world.dim);
    float height = SCALE*world.heightmap[ind];

    bool water = (world.waterpool[ind] > 0);
    height += SCALE*world.waterpool[ind];

    float p = world.waterpath[ind];
    glm::vec3 color;

    if(water) color = waterColor;
    else color = glm::mix(flatColor, waterColor, p);

    glm::vec3 normal = world.normal(glm::ivec2(i, j));
    if(normal.y < steepness && !water)
      color = steepColor;

    vertexpool.fill(section, ind,
      glm::vec3(i, height, j),
      normal,
      vec4(color, 1.0f)
    );

  }

}
