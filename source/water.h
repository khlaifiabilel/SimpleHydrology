struct Drop{
  //Construct Particle at Position
  Drop(glm::vec2 _pos){ pos = _pos; }
  Drop(glm::vec2 _p, glm::ivec2 dim, double v){
    pos = _p;
    int index = _p.x*dim.y+_p.y;
    volume = v;
  }

  //Properties
  int index;
  glm::vec2 pos;
  glm::vec2 speed = glm::vec2(0.0);
  double volume = 1.0;   //This will vary in time
  double sediment = 0.0; //Fraction of Volume that is Sediment!

  //Parameters
  const float dt = 1.2;
  const double density = 1.0;  //This gives varying amounts of inertia and stuff...
  const double evapRate = 0.001;
  const double depositionRate = 0.1;
  const double minVol = 0.01;
  const double friction = 0.15;
  const double volumeFactor = 100.0; //"Water Deposition Rate"

  //Sedimenation Process
  void descend(double* h, double* path, double* pool, bool* track, double* pd, glm::ivec2 dim, double scale);
  void flood(double* h, double* pool, glm::ivec2 dim);
};

glm::vec3 surfaceNormal(int index, double* h, glm::ivec2 dim, double scale){
  glm::vec3 n = glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(h[index]-h[index+dim.y]), 1.0, 0.0));  //Positive X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(h[index-dim.y]-h[index]), 1.0, 0.0));  //Negative X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(h[index]-h[index+1])));    //Positive Y
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(h[index-1]-h[index])));  //Negative Y

  //Diagonals! (This removes the last spatial artifacts)
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index+dim.y+1])/sqrt(2), sqrt(2), scale*(h[index]-h[index+dim.y+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index+dim.y-1])/sqrt(2), sqrt(2), scale*(h[index]-h[index+dim.y-1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index-dim.y+1])/sqrt(2), sqrt(2), scale*(h[index]-h[index-dim.y+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index-dim.y-1])/sqrt(2), sqrt(2), scale*(h[index]-h[index-dim.y-1])/sqrt(2)));    //Positive Y
  return n;
}

void Drop::descend(double* h, double* p, double* b, bool* track, double* pd, glm::ivec2 dim, double scale){

  glm::ivec2 ipos;

  while(volume > minVol){

    //Initial Position
    ipos = pos;
    int ind = ipos.x*dim.y+ipos.y;

    //Add to Path
    track[ind] = true;

    glm::vec3 n = surfaceNormal(ind, h, dim, scale);

    //Effective Parameter Set
    /* Higher plant density means less erosion */
    double effD = depositionRate*max(0.0, 1.0-pd[ind]);

    /* Lower Friction, Lower Evaporation in Streams
    makes particles prefer established streams -> "curvy" */
    double effF = friction*(1.0-0.5*p[ind]);
    double effR = evapRate*(1.0-0.2*p[ind]);

    //Newtonian Mechanics
    glm::vec2 acc = glm::vec2(n.x, n.z)/(float)(volume*density);
    speed += dt*acc;
    pos   += dt*speed;
    speed *= (1.0-dt*effF);

    //New Position
    int nind = (int)(pos.x)*dim.y+(int)(pos.y);

    //Out-Of-Bounds
    if(!glm::all(glm::greaterThanEqual(pos, glm::vec2(0))) ||
       !glm::all(glm::lessThan((glm::ivec2)pos, dim))){
         volume = 0.0;
         break;
       }

    //Particle is not accelerated
    if(p[nind] > 0.5 && length(acc) < 0.01)
      break;

    //Particle enters Pool
    if(b[nind] > 0.0)
      break;

    //Mass-Transfer (in MASS)
    double c_eq = max(0.0, glm::length(speed)*(h[ind]-h[nind]));
    double cdiff = c_eq - sediment;
    sediment += dt*effD*cdiff;
    h[ind] -= volume*dt*effD*cdiff;

    //Evaporate
    volume *= (1.0-dt*effR);
  }
};

void Drop::flood(double* h, double* p, glm::ivec2 dim){

  //Current Height
  index = (int)pos.x*dim.y + (int)pos.y;
  double plane = h[index] + p[index];
  double initialplane = plane;

  //Floodset
  std::vector<int> set;
  int fail = 10;

  //Iterate
  while(volume > minVol && fail){

    set.clear();
    bool tried[dim.x*dim.y] = {false};

    int drain;
    bool drainfound = false;

    std::function<void(int)> fill = [&](int i){

      //Out of Bounds
      if(i/dim.y >= dim.x || i/dim.y < 0) return;
      if(i%dim.y >= dim.y || i%dim.y < 0) return;

      //Position has been tried
      if(tried[i]) return;
      tried[i] = true;

      //Wall / Boundary
      if(plane < h[i] + p[i]) return;

      //Drainage Point
      if(initialplane > h[i] + p[i]){

        //No Drain yet
        if(!drainfound)
          drain = i;

        //Lower Drain
        else if( p[drain] + h[drain] < p[i] + h[i] )
          drain = i;

        drainfound = true;
        return;
      }

      //Part of the Pool
      set.push_back(i);
      fill(i+dim.y);    //Fill Neighbors
      fill(i-dim.y);
      fill(i+1);
      fill(i-1);
      fill(i+dim.y+1);  //Diagonals (Improves Drainage)
      fill(i-dim.y-1);
      fill(i+dim.y-1);
      fill(i-dim.y+1);
    };

    //Perform Flood
    fill(index);

    //Drainage Point
    if(drainfound){

      //Set the Drop Position and Evaporate
      pos = glm::vec2(drain/dim.y, drain%dim.y);

      //Set the New Waterlevel (Slowly)
      double drainage = 0.001;
      plane = (1.0-drainage)*initialplane + drainage*(h[drain] + p[drain]);

      //Compute the New Height
      for(auto& s: set)
        p[s] = (plane > h[s])?(plane-h[s]):0.0;

      //Remove Sediment
      sediment *= 0.1;
      break;
    }

    //Get Volume under Plane
    double tVol = 0.0;
    for(auto& s: set)
      tVol += volumeFactor*(plane - (h[s]+p[s]));

    //We can partially fill this volume
    if(tVol <= volume && initialplane < plane){

      //Raise water level to plane height
      for(auto& s: set)
        p[s] = plane - h[s];

      //Adjust Drop Volume
      volume -= tVol;
      tVol = 0.0;
    }

    //Plane was too high.
    else fail--;

    //Adjust Planes
    initialplane = (plane > initialplane)?plane:initialplane;
    plane += 0.5*(volume-tVol)/(double)set.size()/volumeFactor;
  }

  //Couldn't place the volume (for some reason)- so ignore this drop.
  if(fail == 0)
    volume = 0.0;
}
