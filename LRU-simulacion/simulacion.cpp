#include "LRU.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
	std::cout << "simulacion LRU\n";
	std::cout << std::endl;
	
	int frames;
	std::cout << "frames disponibles: "; std::cin >> frames;
	int paginas;
	std::cout << "cantidad de paginas: "; std::cin >> paginas;
	std::vector<int> secuencia;
	std::cout << "secuencia de paginas: ";
	for (int i=0; i<paginas; i++) {
		int pagina;
		std::cin >> pagina;
		secuencia.push_back(pagina);
	}
	
	SimuladorLRU simulador(frames);
	simulador.simular(secuencia);
	
	std::cout << "\nTabla de simulacion LRU:\n\n";
	simulador.imprimirTabla(std::cout);
	simulador.imprimirDetalle(std::cout);
	simulador.imprimirResumen(std::cout);
	
	return 0;
}
