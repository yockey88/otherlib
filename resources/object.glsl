struct shape {
  int type;
  int idx;
};

struct scene_object {
  /// shape type, shape idx, material idx
  shape sh;
  int material_idx;
};

layout (std140) uniform sphere_buffer {
  sphere spheres[MAX_SPHERES];
};

/// other shapes here
/// ....

layout (std140) uniform object_buffer {
  scene_object objects[MAX_OBJECTS];
};

int get_object_shape_index(int idx) {
  return objects[idx].sh.idx;
}

int get_object_shape_type(int idx) {
  return objects[idx].sh.type;
}

int get_object_material_index(int idx) {
  return objects[idx].material_idx;
}

material get_object_material(int object_idx) {
  return materials[get_object_material_index(object_idx)];
}

void intersect_object(ray r, interval range, inout intersection_record record) {
  float closest_t = infinity;
  int closest_idx = -1;
  intersection_record closest_rec;

  for (int i = 0; i < get_num_objects(); ++i) {
    
    intersection_record temp;
    if (get_object_shape_type(i) == SPHERE_TYPE) {
      sphere s = spheres[get_object_shape_index(i)];

      intersect_sphere(s, r, range, temp);
    }
    else {

    }

    if (temp.hit && temp.t < closest_t)  {
      closest_t = temp.t;
      closest_idx = i;
      closest_rec = temp;
    }
  }

  if (closest_idx >= 0) {
    record = closest_rec;
    record.idx = closest_idx;
    check_face_orientation(record, r, record.normal);
  } 
  else {
    record.hit = false;
  }
}
