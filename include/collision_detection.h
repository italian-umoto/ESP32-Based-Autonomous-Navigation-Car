#pragma once

struct CollisionReading {
    int background;
    int illuminated;
    int reflected;
    bool imminent_collision;
};

void setupCollisionDetection();
CollisionReading readCollisionDetection();
