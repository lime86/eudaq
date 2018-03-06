#include "HexaPos.h"
#include <iostream>

int main(int argc, char* argv[]){

    //////////////////////////////////
    std::cout << "Start HexaPos" << std::endl;
    // create grid
    double cX = 0;//1000.;
    double cY = 0;//2000.;

    HexaPos hexgrid(cX, cY);

    hexgrid.BuildGrid();

    std::cout << "Number of positions " << hexgrid.getNpos() << std::endl;

    for (unsigned int i=0; i < hexgrid.getNpos(); ++i){
    //for (unsigned int i=0; i < std::min(100,hexgrid.getNpos()); ++i){
	//std::cout << "Pos \t" << i << "\tx\t" << hexgrid.getPosX(i) << "\ty\t" << hexgrid.getPosY(i) << std::endl;
	std::cout << i << "\t" << hexgrid.getPosX(i) << "\t" << hexgrid.getPosY(i) << std::endl;
    }

    return 0;
}
