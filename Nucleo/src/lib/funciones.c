#include "funciones.h"

void iniciarEscuchaDeInotify(){
	if (watch_descriptor > 0 && fd_inotify > 0) {
		inotify_rm_watch(fd_inotify, watch_descriptor);
	}
	fd_inotify = inotify_init();
		if (fd_inotify > 0) {
			watch_descriptor = inotify_add_watch(fd_inotify, RUTA_CONFIG_NUCLEO, IN_MODIFY);
		}
}

int setearValoresDeConfig(t_config * archivoConfig){

	config = (t_configuracion*)malloc(sizeof(t_configuracion));

		if (config_has_property(archivoConfig, "PUERTO_UMC")) {
			config->puertoUMC = config_get_int_value(archivoConfig, "PUERTO_UMC");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave PUERTO_UMC.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "IP_UMC")) {
			config->ipUMC = strdup(config_get_string_value(archivoConfig, "IP_UMC"));
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave IP_UMC.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "PUERTO_PROG")) {
			config->puertoPrograma = config_get_int_value(archivoConfig, "PUERTO_PROG");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave PUERTO_PROG.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "PUERTO_CPU")) {
			config->puertoCPU = config_get_int_value(archivoConfig, "PUERTO_CPU");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave PUERTO_CPU.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "QUANTUM")) {
			config->quantum = config_get_int_value(archivoConfig, "QUANTUM");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave QUANTUM.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "QUANTUM_SLEEP")) {
			config->retardoQuantum = config_get_int_value(archivoConfig, "QUANTUM_SLEEP");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave QUANTUM_SLEEP.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "SEM_IDS")) {
			config->semaforosID = config_get_array_value(archivoConfig, "SEM_IDS");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave SEM_IDS.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "SEM_INIT")) {
			config->semaforosValInicial = config_get_array_value(archivoConfig, "SEM_INIT");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave SEM_INIT.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "IO_IDS")) {
			config->ioID = config_get_array_value(archivoConfig, "IO_IDS");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave IO_IDS");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "IO_SLEEP")) {
			config->retardosIO = config_get_array_value(archivoConfig, "IO_SLEEP");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave IO_SLEEP");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "SHARED_VARS")) {
			config->variablesCompartidas = config_get_array_value(archivoConfig, "SHARED_VARS");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave SHARED_VARS.\n");
			return FALSE;
		}
		if (config_has_property(archivoConfig, "STACK_SIZE")) {
			config->cantidadPaginasStack = config_get_int_value(archivoConfig, "STACK_SIZE");
		} else {
			log_info(logger, "El archivo de configuracion no contiene la clave STACK_SIZE.\n");
			return FALSE;
		}
		config_destroy(archivoConfig); // Libero la estructura archivoConfig

	return TRUE;
}

t_semaforo* semaforo_create(char*nombre, int valor){

  t_semaforo *semaforo = malloc(sizeof(t_semaforo));
  semaforo->nombre = strdup(nombre);
  semaforo->valor = valor;
  semaforo->bloqueados = queue_create();

  return semaforo;
}

void registrarSemaforo(char* name, int value){
	t_semaforo* sem = semaforo_create(name, value);
		dictionary_put(diccionarioSemaforos, sem->nombre, sem);
}

var_compartida* crearVariableCompartida(char* nombre, int valorInicial){
	var_compartida* var = malloc(strlen(nombre)+ 5);
		var->nombre = strdup(nombre);
		var->valor = valorInicial;

	return var;
}

void registrarVariableCompartida(char* name, int value){
	var_compartida* var = crearVariableCompartida(name, value);
		dictionary_put(diccionarioVarCompartidas,var->nombre, var);
}

hiloIO* crearHiloIO(int index){
  hiloIO *hilo = malloc(sizeof(hiloIO));
  	  hilo->dataHilo.retardo = atoi(config->retardosIO[index]);
  	  sem_init(&hilo->dataHilo.sem_io, 0, 0);
  	  	  pthread_mutex_init(&hilo->dataHilo.mutex_io, NULL );
  	  hilo->dataHilo.bloqueados = queue_create();
  	  hilo->dataHilo.nombre = strdup(config->ioID[index]);

  return hilo;
}

proceso_bloqueadoIO* esperarPorProcesoIO(dataDispositivo* datos){
	sem_wait(&datos->sem_io);
	pthread_mutex_lock(&datos->mutex_io);
		proceso_bloqueadoIO* proceso = queue_pop(datos->bloqueados);
	pthread_mutex_unlock(&datos->mutex_io);

	return proceso;
}

void encolarPcbAListos(pcb* proceso){
  	  log_info(logger,"Moviendo al proceso #%d a la cola de Listos", proceso->pid);
  queue_push(colaListos, proceso);
}

void* entradaSalidaThread(void* dataHilo){
	int tiempoDeEspera;
	dataDispositivo *datos = dataHilo;
	proceso_bloqueadoIO* pcb_block;
		while (TRUE){
			pcb_block = esperarPorProcesoIO(datos); // tomo el primer proceso en la cola de espera del dispositivo
			tiempoDeEspera = (datos->retardo * pcb_block->espera) * 1000; // duración de la E/S
			usleep(tiempoDeEspera);
			encolarPcbAListos(pcb_block->proceso); // agrego al proceso a la cola de listos
			free(pcb_block); pcb_block = NULL;
		}
		return NULL ;
}

int obtenerSocketMaximoInicial(){
	int max_fd_inicial = 0;

	if (fdEscuchaCPU > fdEscuchaConsola) {
		max_fd_inicial = fdEscuchaCPU;
		} else  {
			max_fd_inicial = fdEscuchaConsola;
		}
	if (max_fd_inicial < fd_inotify) {
		max_fd_inicial = fd_inotify;
	}
	return max_fd_inicial;
}

void planificarProceso(){

	pthread_mutex_lock(&mutex_planificarProceso);

	int i, asignado = 0;
	for (i = 0; (i < list_size(listaCPU) && asignado==0); i++){
		// Recorro buscando algún CPU libre:
		cpu * unCPU = (cpu *)list_get(listaCPU, i);
		if (unCPU->disponibilidad == LIBRE){
			// Si hay algún proceso listo para ejecutar...
			if (queue_size(colaListos) > 0){
				// Saco el primer proceso de la cola:
				pcb * unPCB = (pcb *)queue_pop(colaListos);
				unPCB->id_cpu = unCPU->id;
				unCPU->disponibilidad = OCUPADO;

		aplicar_protocolo_enviar(unCPU->fd_cpu, PCB, unPCB);

		log_info(logger, "El proceso #%i ingresa a ejecución en CPU id #%i.", unPCB->pid, unCPU->id);

		asignado = 1;
				}
			}
		}
	pthread_mutex_unlock(&mutex_planificarProceso);
}

pcb * buscarProcesoPorPid(int pid){
	int i;
	pcb * unPcb;
	for (i = 0; i < list_size(listaProcesos); i++){
		unPcb = (pcb *)list_get(listaProcesos, i);
		if(unPcb->pid == pid){
			return unPcb; // la pcb del proceso es unPcb
		}
	}
	return NULL; // no se encontró el proceso
}

int asignarPid(){
	int randomPid = rand() % 1000; // número aleatorio entre 0 y 1000
	while (buscarProcesoPorPid(randomPid) != NULL){
		randomPid = rand() % 1000;
	}
	return randomPid;
}

int solicitarSegmentosAUMC(pcb* nuevoPcb, char* programa){

	nuevoPcb->pid = asignarPid();
	int tam_prog = strlen(programa)+1;
	int aux_div = tam_prog / tamanioPagina;
	int resto_div = tam_prog % tamanioPagina;
	nuevoPcb->paginas_codigo = (resto_div==0)?aux_div:aux_div+1;
	nuevoPcb->paginas_stack = config->cantidadPaginasStack;

	// Le solicito a UMC espacio para el heap del programa y verifico su respuesta:
		inicioPrograma* solicitudDeInicio = (inicioPrograma*)malloc(tam_prog+8);
		solicitudDeInicio->paginas = nuevoPcb->paginas_stack + nuevoPcb->paginas_codigo;
		solicitudDeInicio->pid = nuevoPcb->pid;
		solicitudDeInicio->contenido = strdup(programa);
		printf("Solicitando segmentos de código y de stack a UMC para el proceso #%d.\n", nuevoPcb->pid);
		aplicar_protocolo_enviar(fd_UMC, INICIAR_PROGRAMA, solicitudDeInicio);
		free(solicitudDeInicio->contenido);
		free(solicitudDeInicio);

		int* respuestaUMC = NULL;
		int head;
		void * entrada = aplicar_protocolo_recibir(fd_UMC, &head);
		if(entrada == NULL){ // UMC mandó un msj vacío, significa que se desconectó
			seDesconectoUMC = true;

			free(respuestaUMC);
			free(entrada);

			return FALSE;
		}
		if(head == RESPUESTA_INICIO_PROGRAMA){
			respuestaUMC = (int*)entrada;
			free(entrada);
		}
		// Verifico la respuesta de UMC:
		if(*respuestaUMC == CREADO){
			printf("UMC pudo alocar todos los segmentos del proceso #%d.\n", nuevoPcb->pid);
			free(respuestaUMC);

			return TRUE;
		}
		else{
			printf("UMC no pudo alocar los segmentos del proceso #%d. Rechazando ingreso al sistema.\n", nuevoPcb->pid);
			free(respuestaUMC);

			return FALSE;
		}
}

pcb * crearPcb(char* programa){

	pcb * nuevoPcb = malloc(sizeof(pcb));
	int respuestaUMC = solicitarSegmentosAUMC(nuevoPcb, programa);
	if(respuestaUMC == FALSE){ // el programa es rechazado del sistema
		free(nuevoPcb);
		return NULL;
	}
	else{
		// La UMC pudo alocar los segmentos del proceso, entonces sigo:
			nuevoPcb->id_cpu = -1; // CPU aún no asignado
			nuevoPcb->quantum = config->quantum;
			nuevoPcb->quantum_sleep = config->retardoQuantum;
/*
		// Analizo con el parser el código del programa para obtener su metadata:
			t_metadata_program* infoProg = metadata_desde_literal(programa);

			nuevoPcb->paginaActualCodigo = 0; // el código empieza en la página # 0
			nuevoPcb->primerPaginaStack = nuevoPcb->paginas_codigo;
			nuevoPcb->paginaActualStack = nuevoPcb->primerPaginaStack;
			nuevoPcb->pc = infoProg->instruccion_inicio; // la siguiente al begin
			nuevoPcb->stackPointer = 0; // offset total en memoria; 0 porque aún está vacío
			nuevoPcb->cantidad_instrucciones = infoProg->instrucciones_size;

		// Inicializo índice de código:
			nuevoPcb->tamanioIndiceCodigo = sizeof(t_intructions) * infoProg->instrucciones_size;
			nuevoPcb->indiceCodigo = infoProg->instrucciones_serializado;

		// Inicializo índice de stack:
			nuevoPcb->indiceStack = list_create();
			nuevoPcb->numeroContextoEjecucionActualStack = 0; // resgistro actual en uso; 0 porque aún está vacío

		// Inicializo índice de etiquetas:
			if (infoProg->cantidad_de_etiquetas > 0 || infoProg->cantidad_de_funciones > 0) {
				nuevoPcb->indiceEtiquetas = strdup(infoProg->etiquetas);
				nuevoPcb->tamanioIndiceEtiquetas = infoProg->etiquetas_size;
			} else {
				nuevoPcb->indiceEtiquetas = NULL;
				nuevoPcb->tamanioIndiceEtiquetas = 0;
			}
			metadata_destruir(infoProg);
*/ // TODO descomentar
			return nuevoPcb;
	}
}

void aceptarConexionEntranteDeConsola(){

	// Las Conosolas mandan un único msj con el script ni bien se conectan:
	 int new_fd = aceptarConexionSocket(fdEscuchaConsola);

	int ret_handshake = handshake_servidor(new_fd, "N");
	if(ret_handshake == FALSE){ // Falló el handshake
		printf("[ERROR] Conexión inicial fallida con la Consola fd #%d.\n", new_fd);
		cerrarSocket(new_fd);
	}
	// Se conectó una nueva Consola:
	consola * nuevaConsola = malloc(sizeof(consola));
	nuevaConsola->id = new_fd - fdEscuchaConsola;
	nuevaConsola->fd_consola = new_fd;
	log_info(logger,"La Consola id #%i se ha conectado.", nuevaConsola->id);

	// Recibo el programa de la nueva Consola:
	int head;
	void * entrada = aplicar_protocolo_recibir(new_fd, &head);
	if(head == ENVIAR_SCRIPT){

	 // Creo la PCB del programa y pido espacio para los segmentos a UMC:
		pcb * nuevoPcb = crearPcb((char*) entrada);
		free(entrada);

		if(nuevoPcb == NULL){ //  UMC no pudo alocar los segmentos del programa, entonces lo rachazo:
			aplicar_protocolo_enviar(new_fd, RECHAZAR_PROGRAMA, MSJ_VACIO);
		  }
	// Sí se pudieron alocar los segmentos, entonces la Consola y el PCB ingresan al sistema:
		nuevaConsola->pid = nuevoPcb->pid;

		list_add(listaConsolas, nuevaConsola);

	// Pongo al nuevo programa en la cola de Listos:
		list_add(listaProcesos, nuevoPcb);
		queue_push(colaListos, nuevoPcb);

		planificarProceso();

		  }
		  else{
			  printf("Se espera script de la Consola #%d.", nuevaConsola->id);
		  }
}

void aceptarConexionEntranteDeCPU(){

	int new_fd = aceptarConexionSocket(fdEscuchaCPU);

	int ret_handshake = handshake_servidor(new_fd, "N");
	if(ret_handshake == FALSE){ // Falló el handshake
		printf("[ERROR] Conexión inicial fallida el CPU fd #%d.\n", new_fd);
		cerrarSocket(new_fd);
	}
	// Se conectó un nuevo CPU:
	cpu * nuevoCPU = malloc(sizeof(cpu));
	nuevoCPU->id = new_fd - fdEscuchaCPU;
	nuevoCPU->fd_cpu = new_fd;
	nuevoCPU->disponibilidad = LIBRE;

	// Agrego al CPU a la lista de CPUs:
	list_add(listaCPU, nuevoCPU);
	log_info(logger,"La CPU %i se ha conectado", nuevoCPU->id);

	// Le envío el tamaño de stack a usar:
	int * stack_size = malloc(INT);
	*stack_size = config->cantidadPaginasStack;
	aplicar_protocolo_enviar(new_fd, TAMANIO_STACK, stack_size);
	free(stack_size);

	// El nuevo CPU está listo para ejecutar procesos
	planificarProceso();
}

void atenderCambiosEnArchivoConfig(int* socketMaximo){

	int length, i = 0;
	char buffer[EVENT_BUF_LEN];
	length = read(fd_inotify, buffer, EVENT_BUF_LEN);

	if (length < 0){
		log_info(logger, "Error al leer el archivo de configuración (inotify_read).");
	} else if (length > 0) {
		struct inotify_event *event = (struct inotify_event *) &buffer[i];
		t_config * new_config = config_create(RUTA_CONFIG_NUCLEO);

			if (new_config && config_has_property(new_config, "QUANTUM")
					&& config_has_property(new_config, "QUANTUM_SLEEP")){

		config->quantum = config_get_int_value(new_config, "QUANTUM");
		config->retardoQuantum = config_get_int_value(new_config, "QUANTUM_SLEEP");
		log_info(logger, "Se ha modificado el archivo de configuración. Quantum: %d - Quantum Sleep: %d",
				config->quantum, config->retardoQuantum);

		i += EVENT_SIZE + event->len;
		free(new_config);

		FD_CLR(fd_inotify, &readfds);
		iniciarEscuchaDeInotify();

		*socketMaximo = (*socketMaximo < fd_inotify)?fd_inotify:*socketMaximo;
				FD_SET(fd_inotify, &readfds);
			}
		}
}

void salvarProcesoEnCPU(int id_cpu){

	bool esLaPcbEnEjecucion(pcb * pcb){ return pcb->id_cpu == id_cpu; }

	pcb * unPcb = (pcb *) list_find(listaProcesos, (void *) esLaPcbEnEjecucion);
	if(unPcb != NULL){
		unPcb->id_cpu = -1; // le desasigno CPU
		queue_push(colaListos, unPcb); // la mando de nuevo a cola de listos

		planificarProceso();
	}
}

int envioSenialCPU(int id_cpu){
	int i;
	for(i=0; i<list_size(listaCPU_SIGUSR1); i++){
		int * cpu = list_get(listaCPU_SIGUSR1, i);
		if(*cpu == id_cpu){ // mandó señal
			free(list_remove(listaCPU_SIGUSR1, i));
			free(cpu);
			return TRUE;
			}
		free(cpu);
	}
	return FALSE; // no mandó señal
}

int pcbListIndex(int pid){
	int i;
	pcb * unPcb;
	for (i = 0; i < list_size(listaProcesos); i++){
		unPcb = (pcb *)list_get(listaProcesos, i);
		if(unPcb->pid == pid){
			free(unPcb);
			return i; // el proceso está en la posición 'i'
		}
			free(unPcb);
	}
	return ERROR; // no se encontró el proceso
}

void finalizarPrograma(int pid, int index){

	if (index != -1){
		log_info(logger,"Liberarndo memoria asignada al proceso #%d.", pid);
		// Aviso a UMC que libere la memoria asignada al proceso:
		int* exit_pid = malloc(INT);
		*exit_pid = pid;
		aplicar_protocolo_enviar(fd_UMC, FINALIZAR_PROGRAMA, exit_pid);
		free(exit_pid);
	}
}

void realizarEntradaSalida(pcb* pcbEjecutada, pedidoIO* datos){

	proceso_bloqueadoIO* pcbIO = malloc(sizeof *pcbIO);
	hiloIO* dispositivoIO = dictionary_get(diccionarioIO, datos->nombreDispositivo);
		pcbIO->espera = datos->tiempo;
		pcbIO->proceso = pcbEjecutada; // debería hacer memcpy ¿?

		// Bloqueo al proceso por IO:
		pthread_mutex_lock(&dispositivoIO->dataHilo.mutex_io);
		queue_push(dispositivoIO->dataHilo.bloqueados, pcbIO);
		pthread_mutex_unlock(&dispositivoIO->dataHilo.mutex_io);
		sem_post(&dispositivoIO->dataHilo.sem_io);
}

void semaforo_signal(t_semaforo* semaforo){
	semaforo->valor++;
	if (semaforo->valor <= 0){
      pcb* procesoBloqueado = queue_pop(semaforo->bloqueados);
      if (procesoBloqueado != NULL){
          encolarPcbAListos(procesoBloqueado);
          planificarProceso();
          // free(procesoBloqueado); ¿?
        }
    }
}

int semaforo_wait(t_semaforo* semaforo){
	semaforo->valor--;
	if (semaforo->valor < 0){
      return TRUE;
    }
  return FALSE;
}

void semaforo_blockProcess(t_queue* colaBloqueados, pcb* proceso){
	queue_push(colaBloqueados, proceso);
}

void recorrerListaCPUsYAtenderNuevosMensajes(){

	int i;
	for (i = 0; i < list_size(listaCPU); i++){

		cpu * unCPU = (cpu *)list_get(listaCPU, i);
		int fd = unCPU->fd_cpu;

		bool consolaTieneElPidCPU(void* unaConsola){ return (((consola*) unaConsola)->pid) == unCPU->pid;}

		if (FD_ISSET(fd , &readfds)){
			// Recibo el nuevo mensaje del CPU:
			int protocolo;
		    void * mensaje = aplicar_protocolo_recibir(fd, &protocolo);

		    if (mensaje == NULL){ // Si el msj en NULL, quito al CPU del sistema y salvo la PCB:
		    	salvarProcesoEnCPU(unCPU->id);
		    	cerrarSocket(fd);
		    	log_info(logger,"La CPU %i se ha desconectado. Salvando PCB en ejecución.", unCPU->id);
		    	free(list_remove(listaCPU, i));
		    }else{
		    	// El mensaje no es NULL, entoces veo de qué se trata:
	switch(protocolo){

	case PCB_FIN_QUANTUM:{

		pcb * pcbEjecutada = (pcb*) mensaje;
		log_info(logger, "Proceso #%i fin de quantum en CPU %i.", pcbEjecutada->pid, unCPU->id);
		if(envioSenialCPU(unCPU->id)){
			log_info(logger, "El CPU %i fue quitado del sistema por señal SIGUSR1.", unCPU->id);
			cerrarSocket(fd);
			free(list_remove(listaCPU, i));
		}
		else{
			// El CPU pasa de Ocupado a Libre:
			unCPU->disponibilidad = LIBRE;
		}
		pcbEjecutada->id_cpu = -1; // le desasigno CPU

		queue_push(colaListos, pcbEjecutada);

		planificarProceso();
		// liberarPcb(pcbEjecutada); ¿?

		break;
	}
	case PCB_FIN_EJECUCION:{

		pcb * pcbEjecutada = (pcb*) mensaje;
		log_info(logger, "Proceso #%i fin de ejecución en CPU %i.", pcbEjecutada->pid, unCPU->id);
		if(envioSenialCPU(unCPU->id)){
			log_info(logger, "El CPU %i fue quitado del sistema por señal SIGUSR1.", unCPU->id);
			cerrarSocket(fd);
			free(list_remove(listaCPU, i));
		}
		else{
			// El CPU pasa de Ocupado a Libre:
			unCPU->disponibilidad = LIBRE;
		}
		pcbEjecutada->id_cpu = -1; // le desasigno CPU
		int index = pcbListIndex(pcbEjecutada->pid); // indexo el pcb en la lista de procesos
		// Le informo a UMC que libere la memoria asignada al programa:
		finalizarPrograma(pcbEjecutada->pid, index);
		// Le informo a la Consola asociada:
		bool consolaTieneElPid(void* unaConsola){ return (((consola*) unaConsola)->pid) == pcbEjecutada->pid;}
		consola * consolaAsociada = list_remove_by_condition(listaConsolas, consolaTieneElPid);
		aplicar_protocolo_enviar(consolaAsociada->fd_consola, FINALIZAR_PROGRAMA, MSJ_VACIO);
		// Libero la Consola asociada y la saco del sistema:
		liberarConsola(consolaAsociada);
		// Libero el PCB del proceso y lo saco del sistema:
		liberarPcb(list_remove(listaProcesos, index));
		liberarPcb(pcbEjecutada);

		planificarProceso();

		break;
	}
	case ENTRADA_SALIDA:{
		// Recibo primero los datos del pedido de IO:
		pedidoIO* datos = (pedidoIO*)mensaje;
		pcb* pcbEjecutada = NULL;
		int head;
		// Espero luego la PCB en ejecución:
		void* entrada = aplicar_protocolo_recibir(fd, &head);
		if(head == PCB_ENTRADA_SALIDA){
			pcbEjecutada = (pcb*)entrada;
		}
		log_info(logger, "Proceso #%i entrada salida en CPU %i.", pcbEjecutada->pid, unCPU->id);
		// El CPU pasa de Ocupada a Libre:
		unCPU->disponibilidad = LIBRE;
		pcbEjecutada->id_cpu = -1; // le desasigno CPU

		realizarEntradaSalida(pcbEjecutada, datos);

		planificarProceso();

		// liberarPcb(pcbEjecutada); ¿?
		free(entrada);
		free(datos);

		break;
	}
	case IMPRIMIR:{

		consola * consolaAsociada = list_find(listaConsolas, (void *)consolaTieneElPidCPU);
		// Le mando el msj a la Consola asociada:
		aplicar_protocolo_enviar(consolaAsociada->fd_consola, IMPRIMIR, string_itoa(*((int*) mensaje)));
		log_info(logger, "Proceso #%i solicita imprimir variable en CPU %i.", unCPU->pid, unCPU->id);

		break;
	}
	case IMPRIMIR_TEXTO:{

		consola * consolaAsociada = list_find(listaConsolas, (void *)consolaTieneElPidCPU);
		// Le mando el msj a la Consola asociada:
		aplicar_protocolo_enviar(consolaAsociada->fd_consola, IMPRIMIR_TEXTO, mensaje);
		log_info(logger, "Proceso #%i solicita imprimir texto en CPU %i.", unCPU->pid, unCPU->id);

		break;
	}
	case ABORTO_PROCESO:{ // Es lo mismo que para fin de ejecución

		int* pid = (int*)mensaje;
		int index = pcbListIndex(*pid);
		log_info(logger, "Proceso #%i abortando ejecución en CPU %i.", *pid, unCPU->id);
		// Le informo a UMC que libere la memoria asignada al programa:
		finalizarPrograma(*pid, index);
		// Le informo a la Consola asociada:
		bool consolaTieneElPidProceso(void* unaConsola){ return (((consola*) unaConsola)->pid) == *pid;}
		consola * consolaAsociada = list_remove_by_condition(listaConsolas, consolaTieneElPidProceso);
		aplicar_protocolo_enviar(consolaAsociada->fd_consola, FINALIZAR_PROGRAMA, MSJ_VACIO);
		// Libero la Consola asociada y la saco del sistema:
		liberarConsola(consolaAsociada);
		// Libero el PCB del proceso y lo saco del sistema:
		liberarPcb(list_remove(listaProcesos, index));
		free(pid);

		planificarProceso();

		break;
	}
	case SIGNAL_REQUEST:{

		t_semaforo* semaforo = dictionary_get(diccionarioSemaforos, (char*)mensaje);
		semaforo_signal(semaforo);

		break;
	}
	case WAIT_REQUEST:{

		t_semaforo* semaforo = dictionary_get(diccionarioSemaforos, (char*)mensaje);
		if (semaforo_wait(semaforo)){ // El proceso se bloquea al hacer wait del semáforo
			// Notifico al CPU:
			aplicar_protocolo_enviar(fd, WAIT_CON_BLOQUEO, MSJ_VACIO);
			pcb* waitPcb = NULL;
			int head;
			// Recibo la PCB en ejecución que se bloqueó:
			void* entrada = aplicar_protocolo_recibir(fd, &head);
			if(head == PCB_WAIT){
				waitPcb = (pcb*)entrada;
			}
			free(entrada);
			semaforo_blockProcess(semaforo->bloqueados, waitPcb);
			// free(waitPcb); ¿?
		}
		else{ // El proceso no se bloquea y puede seguir ejecutando
			aplicar_protocolo_enviar(fd, WAIT_SIN_BLOQUEO, MSJ_VACIO);
		}
		// free(semaforo); ¿?

		break;
	}
	case OBTENER_VAR_COMPARTIDA:{

		// Recibo un char* y devuelvo un int:
		var_compartida* varPedida = dictionary_get(diccionarioVarCompartidas, (char*)mensaje);
		aplicar_protocolo_enviar(fd, DEVOLVER_VAR_COMPARTIDA, &(varPedida->valor));
		free(varPedida->nombre);
		free(varPedida);

		break;
	}
	case GRABAR_VAR_COMPARTIDA:{

		var_compartida* var_aGrabar = (var_compartida*)mensaje;
		// Actualizo el valor de la variable solicitada:
		var_compartida* varBuscada = dictionary_get(diccionarioVarCompartidas, var_aGrabar->nombre);
		varBuscada->valor = var_aGrabar->valor;
		free(var_aGrabar->nombre);
		free(var_aGrabar);

		break;
	}
	case SIGUSR1: {

		log_info(logger, "Señal SIGUSR1 del CPU %i. Será quitado del sistema al finalizar ráfaga de ejecución actual.", unCPU->id);
		list_add(listaCPU_SIGUSR1, &(unCPU->id));

		break;
	}
	default:
		printf("El head #%d del mensaje no es reconocido por el Núcleo.\n", protocolo);
		break;
	} // fin switch protocolo
	free(mensaje); mensaje = NULL;
		    } // fin else mensaje not null
		} // fin if nuevo mensaje fd CPU
	} // fin for listaCPU
}

void liberarCPU(cpu * cpu){ free(cpu); cpu = NULL; }

void liberarConsola(consola * consola){ free(consola); consola = NULL;}

void liberarSemaforo(t_semaforo * sem){
	free(sem->nombre);
	queue_destroy_and_destroy_elements(sem->bloqueados, (void *) liberarPcb);
	sem->bloqueados = NULL;
	free(sem); sem = NULL; }

void liberarVarCompartida(var_compartida * var){ free(var->nombre); free(var); var = NULL; }

void limpiarColecciones(){
	list_destroy_and_destroy_elements(listaProcesos,(void *) liberarPcb);
	listaProcesos = NULL;

	list_destroy_and_destroy_elements(listaCPU,(void *) liberarCPU);
	listaCPU = NULL;

	list_clean(listaCPU_SIGUSR1);
	list_destroy(listaCPU_SIGUSR1);
	listaCPU_SIGUSR1 = NULL;

	list_destroy_and_destroy_elements(listaCPU,(void *) liberarConsola);
	listaConsolas = NULL;

	queue_destroy_and_destroy_elements(colaListos, (void *) liberarPcb);
	colaListos = NULL;

	dictionary_destroy_and_destroy_elements(diccionarioSemaforos, (void *) liberarSemaforo);
	diccionarioSemaforos = NULL;

	dictionary_destroy_and_destroy_elements(diccionarioVarCompartidas, (void *) liberarVarCompartida);
	diccionarioVarCompartidas = NULL;
}

void limpiarArchivoConfig(){
	free(config->ioID);
	free(config->ipUMC);
	free(config->retardosIO);
	free(config->semaforosID);
	free(config->semaforosValInicial);
	free(config->variablesCompartidas);
	free(config);
	config = NULL;
}
