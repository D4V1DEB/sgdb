#ifndef LRU_H
#define LRU_H

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct EstadoPaso {
	int paginaSolicitada;
	std::vector<int> paginas;
	std::vector<int> anterior;
	bool fallo;
	int paginaReemplazada;
};

class SimuladorLRU {
private:
	int cantidadFrames;
	std::vector<int> frames;
	std::unordered_map<int, int> ultimoUso;
	std::vector<EstadoPaso> historial;
	int hits;
	int fallos;
	
	int buscarFrame(int pagina) const {
		for (int i = 0; i < static_cast<int>(frames.size()); ++i) {
			if (frames[i] == pagina) {
				return i;
			}
		}
		return -1;
	}
	
	int buscarFrameLibre() const {
		for (int i = 0; i < cantidadFrames; ++i) {
			if (frames[i] == -1) {
				return i;
			}
		}
		return -1;
	}
	
	int buscarVictimaLRU(int pasoActual) const {
		int indiceVictima = 0;
		int mayorAnterior = -1;
		
		for (int i = 0; i < cantidadFrames; ++i) {
			int pagina = frames[i];
			int anterior = pasoActual - ultimoUso.at(pagina);
			
			if (anterior > mayorAnterior) {
				mayorAnterior = anterior;
				indiceVictima = i;
			}
		}
		
		return indiceVictima;
	}
	
	std::vector<int> calcularAnterior(int pasoActual) const {
		std::vector<int> valores(cantidadFrames, -1);
		
		for (int i = 0; i < cantidadFrames; ++i) {
			if (frames[i] != -1) {
				valores[i] = pasoActual - ultimoUso.at(frames[i]);
			}
		}
		
		return valores;
	}
	
public:
		explicit SimuladorLRU(int framesDisponibles)
			: cantidadFrames(framesDisponibles),
			frames(framesDisponibles, -1),
			hits(0),
			fallos(0) {}
		
		void simular(const std::vector<int>& secuencia) {
			historial.clear();
			ultimoUso.clear();
			std::fill(frames.begin(), frames.end(), -1);
			hits = 0;
			fallos = 0;
			
			for (int paso = 0; paso < static_cast<int>(secuencia.size()); ++paso) {
				int pagina = secuencia[paso];
				int indice = buscarFrame(pagina);
				bool huboFallo = false;
				int reemplazada = -1;
				
				if (indice != -1) {
					++hits;
				} else {
					huboFallo = true;
					++fallos;
					
					int libre = buscarFrameLibre();
					if (libre != -1) {
						frames[libre] = pagina;
					} else {
						int victima = buscarVictimaLRU(paso);
						reemplazada = frames[victima];
						ultimoUso.erase(reemplazada);
						frames[victima] = pagina;
					}
				}
				
				ultimoUso[pagina] = paso;
				
				EstadoPaso estado;
				estado.paginaSolicitada = pagina;
				estado.paginas = frames;
				estado.anterior = calcularAnterior(paso);
				estado.fallo = huboFallo;
				estado.paginaReemplazada = reemplazada;
				historial.push_back(estado);
			}
		}
		
		void imprimirTabla(std::ostream& salida) const {
			const int anchoEtiqueta = 12;
			const int anchoSubColumna = 5;
			
			salida << std::left << std::setw(anchoEtiqueta) << "Demanda";
			for (const EstadoPaso& estado : historial) {
				salida << std::right << std::setw(anchoSubColumna) << estado.paginaSolicitada
					<< std::setw(anchoSubColumna) << "ant.";
			}
			salida << '\n';
			
			salida << std::left << std::setw(anchoEtiqueta) << "/ Frame";
			for (std::size_t i = 0; i < historial.size(); ++i) {
				salida << std::right << std::setw(anchoSubColumna) << "pag"
					<< std::setw(anchoSubColumna) << "prev";
			}
			salida << '\n';
			
			salida << std::string(anchoEtiqueta + historial.size() * anchoSubColumna * 2, '-') << '\n';
			
			for (int frame = 0; frame < cantidadFrames; ++frame) {
				std::ostringstream etiqueta;
				etiqueta << "F" << (frame + 1);
				salida << std::left << std::setw(anchoEtiqueta) << etiqueta.str();
				
				for (const EstadoPaso& estado : historial) {
					if (estado.paginas[frame] == -1) {
						salida << std::right << std::setw(anchoSubColumna) << "-"
							<< std::setw(anchoSubColumna) << "-";
					} else {
						salida << std::right << std::setw(anchoSubColumna) << estado.paginas[frame]
							<< std::setw(anchoSubColumna) << estado.anterior[frame];
					}
				}
				salida << '\n';
			}
			
			salida << std::string(anchoEtiqueta + historial.size() * anchoSubColumna * 2, '-') << '\n';
			salida << std::left << std::setw(anchoEtiqueta) << "Fallo";
			for (const EstadoPaso& estado : historial) {
				salida << std::right << std::setw(anchoSubColumna) << (estado.fallo ? "X" : "")
					<< std::setw(anchoSubColumna) << "";
			}
			salida << '\n';
		}
		
		void imprimirDetalle(std::ostream& salida) const {
			salida << "\nDetalle paso a paso:\n";
			for (std::size_t i = 0; i < historial.size(); ++i) {
				const EstadoPaso& estado = historial[i];
				salida << "Paso " << (i + 1)
					<< " | Pagina " << estado.paginaSolicitada << " | ";
				
				if (estado.fallo) {
					salida << "Fallo de pagina";
					if (estado.paginaReemplazada != -1) {
						salida << " | Sale " << estado.paginaReemplazada;
					}
				} else {
					salida << "Hit";
				}
				
				salida << '\n';
			}
		}
		
		void imprimirResumen(std::ostream& salida) const {
			int total = hits + fallos;
			double porcentajeHits = total == 0 ? 0.0 : (hits * 100.0) / total;
			double porcentajeFallos = total == 0 ? 0.0 : (fallos * 100.0) / total;
			
			salida << "\nResumen:\n";
			salida << "Total de referencias: " << total << '\n';
			salida << "Hits: " << hits << " (" << std::fixed << std::setprecision(2)
				<< porcentajeHits << "%)\n";
			salida << "Fallos de pagina: " << fallos << " (" << std::fixed
				<< std::setprecision(2) << porcentajeFallos << "%)\n";
		}
};

#endif
