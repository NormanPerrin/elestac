#ifndef NUCLEOVIEJOMALDITO_SRC_LIB_PRINCIPALES_H_
#define NUCLEOVIEJOMALDITO_SRC_LIB_PRINCIPALES_H_

#include "funciones.h"

/*** FUNCIONES PRINCIPALES ***/
void inicializarColecciones();
void crearLoggerNucleo();
void leerConfiguracionNucleo();
void llenarDiccionarioSemaforos();
void llenarDiccionarioVarCompartidas();
void lanzarHilosIO();
int conexionConUMC();
void esperar_y_PlanificarProgramas();
void unirHilosIO();
void liberarRecursosUtilizados();
void exitNucleo();
void exitFailureNucleo();

#endif /* NUCLEOVIEJOMALDITO_SRC_LIB_PRINCIPALES_H_ */
