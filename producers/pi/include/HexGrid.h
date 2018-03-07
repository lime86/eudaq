#ifndef HEXAPOS_H
#define HEXAPOS_H

// system includes
#include <vector>

// Hexagonal position list
class HexGrid {
    /*
     _______
    /   |   \
   /    |    \
  /_____|_____\
  \     |     /
   \    |    /
    \   |   /
     -------

    */

protected:

    // Hexagon parameters
    const double m_largeD = 1350.;
    const double m_smallD = 1250.;

    const double m_largeR = m_largeD / 2.;
    const double m_smallR = m_smallD / 2.;

    // side parameters
    // y = ax + b for the top-right edge
    // x = (y-b)/a
    const double m_b =  2. * m_smallR;
    const double m_a = -2. * m_smallR/m_largeR;

    // center positions
    double m_centerX = 0.;
    double m_centerY = 0.;

    // step sizes
    double m_stepSizeX = 50.;
    double m_stepSizeY = 25.;

    // border offset
    double m_borderOffsetX = 0;
    double m_borderOffsetY = 0;

    // start positions
    double m_startY = m_smallR - m_borderOffsetY;
    double m_startX = - (m_startY - m_b) / m_a + m_borderOffsetX;

    // Position list/vector
    std::vector<double> m_positionsX;
    std::vector<double> m_positionsY;

    std::vector< std::vector<double> > m_positions;

    unsigned int m_npoints = 0;

public:

    HexGrid();
    HexGrid(double centerX, double centerY);

    ~HexGrid();

    int BuildGrid();

    // functions to set parameters
    void setStepX(int step) { m_stepSizeX = step; };
    void setStepY(int step) { m_stepSizeY = step; };

    // functions to get parameters
    int getPosX(int positionID) { return (positionID < m_npoints)? m_positionsX.at(positionID):-1; };
    int getPosY(int positionID) { return (positionID < m_npoints)? m_positionsY.at(positionID):-1; };
    int getNpos() {return m_npoints;};
};

#endif
