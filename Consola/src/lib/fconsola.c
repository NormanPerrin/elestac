#include "fconsola.h"


void validar_argumentos(int arg) {
	if(arg != 2) {
		printf("Debe ingresar el archivo a ejecutar como parámetro\n");
		exit(1);
	}
}

void setearValores_config(t_config * archivoConfig){
	puertoNucleo = config_get_int_value(archivoConfig, "PUERTO_NUCLEO");
	ipNucleo = strdup(config_get_string_value(archivoConfig, "IP_NUCLEO"));
}

void leerScript(char * rutaPrograma){
	programa = (t_string*)malloc(sizeof(t_string));

	int _tamanio, descriptorArchivo;
	struct stat infoArchivo; // Ver función 'stat' en stat.h

	descriptorArchivo = open(rutaPrograma, O_RDONLY); // Abre el archivo .asnsisop
		fstat(descriptorArchivo, &infoArchivo); // Obtenemos su información
		_tamanio = infoArchivo.st_size;
		programa->tamanio = _tamanio;

		read(descriptorArchivo, programa->texto, programa->tamanio); // Guardo el script en programa
		close(descriptorArchivo);

		rutaScript = strdup(rutaPrograma); // Extra: se guarda la ruta en una variable global
} // El programa ya está listo para ser enviado a Núcleo

void conectarConNucleo(){
	fd_nucleo = nuevoSocket();
	int ret = conectarSocket(fd_nucleo, ipNucleo, puertoNucleo);
	validar_conexion(ret, 1); // Al ser cliente es terminante
	handshake_cliente(fd_nucleo, "C");

	aplicar_protocolo_enviar(fd_nucleo, ENVIAR_SCRIPT, programa, SIZE_MSG);

	cerrarSocket(fd_nucleo);
}

void testLecturaArchivoDeConfiguracion(){
	printf("Puerto Núcleo: %d\n", puertoNucleo);
	printf("IP Núcleo: %s\n", ipNucleo);
}

void liberarRecusos() {
	free(ipNucleo);
}

int validar_servidor(char *id) {
	if(!strcmp(id, "N")) {
		printf("Servidor aceptado\n");
		return TRUE;
	} else {
		printf("Servidor rechazado\n");
		return FALSE;
	}
}
int validar_cliente(char *id) {return 0;}

void esperar_mensajes() {

	int head;

	while(TRUE) {
		int ret;
		ret = recibirPorSocket(fd_nucleo, &head, 1);
		validar_recive(ret, 1); // es terminante ya que si hay un error en el recive o desconexión debe terminar
		aplicar_protocolo_recibir(fd_nucleo, &head, SIZE_MSG);
	}
}


void imprimir(char *texto) { printf("IMPRIMIR: %s\n", texto); }
