#ifndef CUBICINTP_H
#define CUBICINTP_H

#include "glm/glm.hpp"

class Cubicintp
{
    public:
        static glm::vec3 CatMullRom(const glm::vec3 &p0, 
                                    const glm::vec3 &p1, 
                                    const glm::vec3 &p2, 
                                    const glm::vec3 &p3, 
                                    float t /* between 0 and 1 */, 
                                    float alpha=.5f /* between 0 and 1 */);
};

#endif