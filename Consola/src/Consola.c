#include <stdlib.h>
#include "lib/fconsola.h"

int main(int argc, char **argv){

	system("clear");
	setvbuf(stdout, NULL, _IONBF, 0);

	validar_argumentos(argc);

	crearLoggerConsola();
	leerArchivoDeConfiguracion(RUTA_CONFIG_CONSOLA);

	leerScript(argv[1]);

	conectarCon_Nucleo(); // Conexión con Núcleo

	string* script = malloc(tamanioPrograma + 4);
	script->tamanio = tamanioPrograma;
	script->cadena = programa;
	aplicar_protocolo_enviar(fd_nucleo, ENVIAR_SCRIPT, script);
	printf("Script enviado a Núcleo. Esperando respuesta...\n");
	free(script->cadena); free(script);

	int head;
	void * mensaje = NULL;
	mensaje = aplicar_protocolo_recibir(fd_nucleo, &head); // Recibo respuesta de incio programa

	if (head == PROGRAMA_NEW){

		switch(*((int*) mensaje)){

	case RECHAZADO:{ // programa rechazado
				log_info(logger, "UMC no pudo alocar los segmentos pedidos. Programa rechazado.");
				exitConsola();
				return EXIT_FAILURE;
				break;
			} // fin case rechazado

	case ERROR_CONEXION:{
				log_error(logger, "Error al iniciar programa. Script no enviado.");
				exitConsola();
				return EXIT_FAILURE;
				break;
			} // fin case error conexión

	case ACEPTADO:{ // programa aceptado
				log_info(logger, "Programa aceptado. Escuchando nuevos mensajes de Núcleo...");

		while(TRUE){ // Espera activa de mensajes

				int protocolo;
				void * entrada = NULL;
				entrada = aplicar_protocolo_recibir(fd_nucleo, &protocolo);
				if(entrada == NULL) break;

			switch(protocolo){

			case IMPRIMIR_TEXTO:{
						// Imprime lo que recibe, ya sea texto a variable (convertida a texto):
						printf("IMPRIMIR: '%s'.\n", (char*)entrada);
						break;
					}
			case FINALIZAR_PROGRAMA:{
						int respuesta = *((int*)entrada);
						if(respuesta == PERMITIDO) {
							printf("El programa ha finalizado con éxito.\n");
						} else {
							printf("El programa ha sido abortado.\n");
						}
						exitConsola();
						return EXIT_SUCCESS;
						break;
					}
			default:
				printf("Se ha recibido un mensaje inválido de Núcleo.");
				break;
				} // fin switch-case nuevos mensajes
			} // fin while espera mensajes
					break;
		} // fin case aceptado
	} // fin switch respuesta inicio
		free(mensaje);
	} // fin if head válido

	exitConsola();
	return EXIT_FAILURE;

} // fin main
