/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº: 129850      Nome: Gonçalo Sobral
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:O módulo servidor.c implementa um sistema de gestão de parque de estacionamento baseado 
 ** numa arquitetura pai-filho com comunicação por IPC. O servidor principal (pai) recebe 
 ** pedidos de check-in de clientes através de message queues e cria processos servidores 
 ** dedicados (filhos) para processar cada pedido individualmente. Cada servidor dedicado 
 ** valida os dados do cliente, reserva um lugar no parque, gere o estacionamento durante 
 ** toda a permanência da viatura e processa o check-out quando solicitado.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int nrServidoresDedicados = 0;          // Número de servidores dedicados (só faz sentido no processo Servidor)
int shmId = -1;                         // Variável que tem o ID da Shared Memory
int msgId = -1;                         // Variável que tem o ID da Message Queue
int semId = -1;                         // Variável que tem o ID do Grupo de Semáforos
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento = NULL;   // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD = -1;                // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile = -1;               // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile
int shmIdFACE = -1;                     // Variável que tem o ID da Shared Memory da entidade externa FACE
int semIdFACE = -1;                     // Variável que tem o ID do Grupo de Semáforos da entidade externa FACE
int *tarifaAtual = NULL;                // Inteiro definido pela entidade externa FACE com a tarifa atual do parque

/**
 * @brief  Processamento do processo Servidor e dos processos Servidor Dedicado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @return Success (0) or not (<> 0)
 */
int main(int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv);
    s2_MainServidor();

    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_ArmaSinaisServidor();
    s1_3_CriaMsgQueue(IPC_KEY, &msgId);
    s1_4_CriaGrupoSemaforos(IPC_KEY, &semId);
    s1_5_CriaBD(IPC_KEY, &shmId, dimensaoMaximaParque, &lugaresEstacionamento);

    so_debug(">");
}

/**
 * @brief s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 * @param pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    //verificar nº de argumentos
   if (argc < 2) {
        so_error("S1.1", "Tem de inserir a capacidade máxima do parque como argumento");
        exit(1);
    }


    char *lastptr;
    errno = 0;
    //converter string para long 
    long result = strtol(argv[1], &lastptr, 10);

    //verificar se nº é positivo e se a conversão occorreu sem erros
    if (errno != 0 || *lastptr != '\0' || result <= 0) {
        so_error("S1.1", "O número tem que ser positivo");
        exit(1);
    }

    //dar o valor do tamanho do parque
    *pdimensaoMaximaParque = (int)result;

    so_success("S1.1", "Dimensão validadada : %d lugares.", *pdimensaoMaximaParque);

    so_debug("> [@return *pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief s1_2_ArmaSinaisServidor Ler a descrição da tarefa S1.2 no enunciado
 */
void s1_2_ArmaSinaisServidor() {
    so_debug("<");
    //armar SIGINT
    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar o sinal SIGINT");
        exit(1);
    }
    //armar SIGCHLD
    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar o sinal SIGCHLD");
        exit(1);
    }
    so_success("S1.2", "Sinais armados");


    so_debug(">");
}

/**
 * @brief s1_3_CriaMsgQueue Ler a descrição da tarefa s1.3 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void s1_3_CriaMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    //verificar se existe uma msgQ
    int existing_msgid = msgget(ipcKey, 0);
    if (existing_msgid != -1) {
        if (msgctl(existing_msgid, IPC_RMID, NULL) == -1) {
            so_error("S1.3", "Falha a remover a fila de mensagens que já existia: %s", strerror(errno));
            exit(0);
        }
    }

    //criar msgQ
    *pmsgId = msgget(ipcKey, IPC_CREAT | IPC_EXCL | 0666);
    if (*pmsgId == -1) {
        so_error("S1.3", "Falha a criar a fila de mensagens: %s", strerror(errno));
        exit(0);
    }

    so_success("S1.3", "Fila de mensagens criada com sucesso (msgid=%d)", *pmsgId);


    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief s1_4_CriaGrupoSemaforos Ler a descrição da tarefa s1.4 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param psemId (O) identificador aberto de IPC
 */
void s1_4_CriaGrupoSemaforos(key_t ipcKey, int *psemId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    //verificar se exite um grupo já criado de sem
    int existing_semid = semget(ipcKey, 0, 0666);
    if (existing_semid != -1) {
        //remover se existir
        if (semctl(existing_semid, 0, IPC_RMID) == -1) {
            so_error("S1.4", "Erro a remover grupo de semáforos existente: %s", strerror(errno));
            exit(0);
        }
    }

    //criar semáforos
    *psemId = semget(ipcKey, 4, IPC_CREAT | IPC_EXCL | 0600);
    if (*psemId == -1) {
        so_error("S1.4", "Erro a criar os semáforos: %s", strerror(errno));
        exit(0);
    }

    //iniciar valores dos semáforos
    if (semctl(*psemId, SEM_MUTEX_BD, SETVAL, 1) == -1 ||  
        semctl(*psemId, SEM_SRV_DEDICADOS, SETVAL, 0) == -1 || 
        semctl(*psemId, SEM_MUTEX_LOGFILE, SETVAL, 1) == -1 ||
        semctl(*psemId, SEM_LUGARES_PARQUE, SETVAL, dimensaoMaximaParque) == -1)
         {
        so_error("S1.4", "Erro ao iniciar os semáforos: %s", strerror(errno));
        exit(0);
    }
    so_success("S1.4", "Grupo de semáforos criado (semid=%d)", *psemId);

    so_debug("> [@return *psemId:%d]", *psemId);
}

/**
 * @brief s1_5_CriaBD Ler a descrição da tarefa S1.5 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pshmId (O) identificador aberto de IPC
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void s1_5_CriaBD(key_t ipcKey, int *pshmId, int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param ipcKey:0x0%x, dimensaoMaximaParque:%d]", ipcKey, dimensaoMaximaParque);

    //calcular memória necessária
    size_t tamanhoMemoria = dimensaoMaximaParque * sizeof(Estacionamento);
    int criarNovo = 0;

    //tenta obter uma memória partilhada existente
    *pshmId = shmget(ipcKey, tamanhoMemoria, IPC_GET);

    if (*pshmId == -1) {
        //se não existir cria
        *pshmId = shmget(ipcKey, tamanhoMemoria, IPC_CREAT | 0600);
        criarNovo = 1;
        
        if (*pshmId < 0) {
            so_error("S1.5", "Erro ao criar a Shared Memory: %s", strerror(errno));
            exit(1);
        }
    }

    //anexa a memória partilhada ao espaço de endereçamento do processo
    *plugaresEstacionamento = (Estacionamento *) shmat(*pshmId, NULL, 0);
    
    if (*plugaresEstacionamento == (Estacionamento *) -1) {
        so_error("S1.5", "Erro ao ligar à Shared Memory: %s", strerror(errno));
        exit(1);
    }

    if (criarNovo) {
        //se criou nova memória, inicializa todos os lugares como disponíveis
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
        }
        
        so_success("S1.5", "Shared Memory criada e inicializada com ID %d", *pshmId);
    } else {
        so_success("S1.5", "Ligado à Shared Memory existente com ID %d", *pshmId);
    }
    

    so_debug("> [@return *pshmId:%d, *plugaresEstacionamento:%p]", *pshmId, *plugaresEstacionamento);
}

/**
 * @brief s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s2_MainServidor() {
    so_debug("<");

    while (TRUE) {
        s2_1_LePedidoCliente(msgId, &clientRequest);
        s2_2_CriaServidorDedicado(&nrServidoresDedicados);
    }

    so_debug(">");
}

/**
 * @brief s2_1_LePedidoCliente Ler a descrição da tarefa S2.1 no enunciado.
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) pedido recebido, enviado por um Cliente
 */
void s2_1_LePedidoCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);
    //recebe mensagem do tipo MSGTYPE_LOGIN
    int resultado = msgrcv(msgId, pclientRequest, sizeof(pclientRequest->msgData), MSGTYPE_LOGIN, 0);

    if (resultado < 0) {
        if (errno == EINTR) {
            //se foi interrompido por sinal, retorna sem erro
            return;
        }
        //para outros erros, encerra o servido
        so_error("S2.1", "Erro ao ler mensagem da Message Queue: %s", strerror(errno));
        s4_EncerraServidor();;
        return;
    }

    so_success("S2.1", "%s %d", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.pidCliente);

    //sleep(10);  // TEMPORÁRIO, os alunos deverão comentar este statement apenas
                // depois de terem a certeza que não terão uma espera ativa

    so_debug("> [@return *pclientRequest:[%s:%s:%c:%s:%d.%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief s2_2_CriaServidorDedicado Ler a descrição da tarefa S2.2 no enunciado
 * @param pnrServidoresDedicados (O) número de Servidores Dedicados que foram criados até então
 */
void s2_2_CriaServidorDedicado(int *pnrServidoresDedicados) {
    so_debug("<");
    pid_t pid;
    //cria processo filho
    pid = fork();

    if (pid < 0) {
        //erro na criação do processo
        so_error("S2.2", "Erro ao criar Servidor Dedicado: %s", strerror(errno));
        s4_EncerraServidor();
        return;
    } 
    else if (pid == 0) {
        //código do processo filho - servidor dedicado
        so_success("S2.2", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado(); //função do servidor dedicado
        exit(1);
    }
    else {
        //código do processo pai - servidor principal
        (*pnrServidoresDedicados)++;
        so_success("S2.2", "Servidor: Iniciei SD %d", pid);
        clientRequest.msgData.est.pidServidorDedicado = pid;
    }
    so_debug("> [@return *pnrServidoresDedicados:%d", *pnrServidoresDedicados);
}

/**
 * @brief s3_TrataCtrlC Ler a descrição da tarefa S3 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown");
    s4_EncerraServidor();//inicia encerramento 
    exit(0);

    so_debug(">");
}

/**
 * @brief s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s4_EncerraServidor() {
    so_debug("<");

    s4_1_TerminaServidoresDedicados(lugaresEstacionamento, dimensaoMaximaParque);
    s4_2_AguardaFimServidoresDedicados(nrServidoresDedicados);
    s4_3_ApagaElementosIPCeTermina(shmId, semId, msgId);

    so_debug(">");
}

/**
 * @brief s4_1_TerminaServidoresDedicados Ler a descrição da tarefa S4.1 no enunciado
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 */
void s4_1_TerminaServidoresDedicados(Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque) {
    so_debug("< [@param lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", lugaresEstacionamento, dimensaoMaximaParque);

    //bloqueia acesso à base de dados para leitura segura
    struct sembuf semBufDown = {SEM_MUTEX_BD, -1, 0};
    if (semop(semId, &semBufDown, 1) == -1) {
        so_error("S4.1", "Erro ao adquirir semáforo de acesso à BD: %s", strerror(errno));
        return;
    }

    //percorre todos os lugares e envia sinal de terminação aos servidores dedicados
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidServidorDedicado > 0) {
            int pid = lugaresEstacionamento[i].pidServidorDedicado;
            if (kill(pid, SIGUSR2) == 0) {
                so_success("S4.1", "Enviado sinal SIGUSR2 para o servidor dedicado com PID %d", pid);
            } else {
                so_error("S4.1", "Erro ao enviar sinal SIGUSR2 para o servidor dedicado com PID %d: %s", pid, strerror(errno));
            }
        }
    }

    //liberta o acesso à base de dados
    struct sembuf semBufUp = {SEM_MUTEX_BD, 1, 0};
    if (semop(semId, &semBufUp, 1) == -1) {
        so_error("S4.1", "Erro ao liberar semáforo de acesso à BD: %s", strerror(errno));
    }

    so_debug(">");
}

/**
 * @brief s4_2_AguardaFimServidoresDedicados Ler a descrição da tarefa S4.2 no enunciado
 * @param nrServidoresDedicados (I) número de Servidores Dedicados que foram criados até então
 */
void s4_2_AguardaFimServidoresDedicados(int nrServidoresDedicados) {
    so_debug("< [@param nrServidoresDedicados:%d]", nrServidoresDedicados);

    if (nrServidoresDedicados <= 0) {
        so_success("S4.2", "Não existem servidores dedicados para aguardar");
        so_debug(">");
        return;
    }
 
    so_success("S4.2", "Aguardando a conclusão de %d servidores dedicados", nrServidoresDedicados);

    // Usar o semáforo como barreira
    struct sembuf semBuf = {SEM_SRV_DEDICADOS, -nrServidoresDedicados, 0};
    if (semop(semId, &semBuf, 1) == -1) {
        if (errno == EINTR) {
            so_error("S4.2", "Espera interrompida por sinal");
        } else {
            so_error("S4.2", "Erro ao aguardar pelo semáforo: %s", strerror(errno));
        }
    }
    
    so_success("S4.2", "Todos os servidores dedicados terminaram");

    so_debug(">");
}

/**
 * @brief s4_3_ApagaElementosIPCeTermina Ler a descrição da tarefa S4.2 no enunciado
 * @param shmId (I) identificador aberto de IPC
 * @param semId (I) identificador aberto de IPC
 * @param msgId (I) identificador aberto de IPC
 */
void s4_3_ApagaElementosIPCeTermina(int shmId, int semId, int msgId) {
    so_debug("< [@param shmId:%d, semId:%d, msgId:%d]", shmId, semId, msgId);

    //remove memória partilhada
    if (shmctl(shmId, IPC_RMID, NULL) == -1) {
        so_debug("Falha ao remover SHM: %s", strerror(errno)); 
    }
    //remove grupo de semáforos
    if (semctl(semId, 0, IPC_RMID) == -1) {
        so_debug("Falha ao remover Semáforos: %s", strerror(errno));
    }
    //remove msgqueue
    if (msgctl(msgId, IPC_RMID, NULL) == -1) {
        so_debug("Falha ao remover MSG: %s", strerror(errno));
    }

    so_success("S4.3", "Servidor: End Shutdown");

    exit(0);

    so_debug(">");
}

/**
 * @brief s5_TrataTerminouServidorDedicado Ler a descrição da tarefa S5 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    //recolhe informação sobre o processo filho que terminou
    pid_t pidTerminado = wait(NULL);
    if (pidTerminado == -1) {
        so_error("S5", "wait error: %s", strerror(errno));
        so_debug(">");
        return;
    }

    nrServidoresDedicados--;
    so_success("S5", "Servidor: Confirmo que terminou o SD %d", pidTerminado);

    //bloqueia acesso à base de dados para atualização
    struct sembuf semBufLock = {SEM_MUTEX_BD, -1, 0};
    if (semop(semId, &semBufLock, 1) == -1) {
        so_error("S5", "Failed to lock BD semaphore: %s", strerror(errno));
        so_debug(">");
        return;
    }

    //procura e limpa o registo do servidor dedicado na base de dados
    int found = 0;
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidServidorDedicado == pidTerminado) {
            lugaresEstacionamento[i].pidCliente = DISPONIVEL;
            lugaresEstacionamento[i].pidServidorDedicado = 0;
            found = 1;
            break;
        }
    }

    //liberta acesso à base de dados
    struct sembuf semBufUnlock = {SEM_MUTEX_BD, 1, 0};
    if (semop(semId, &semBufUnlock, 1) == -1) {
        so_error("S5", "Failed to unlock BD semaphore: %s", strerror(errno));
    }

    if (!found) {
        so_error("S5", "Dedicated server %d not found in database", pidTerminado);
    }

    so_debug("> [@return nrServidoresDedicados:%d]", nrServidoresDedicados);
}

/**
 * @brief sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_GetShmFACE(KEY_FACE, &shmIdFACE);
    sd7_4_GetSemFACE(KEY_FACE, &semIdFACE);
    sd7_5_ProcuraLugarDisponivelBD(semId, clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);
    
    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSucessoAoCliente(msgId, clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout(msgId);
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief sd7_1_ArmaSinaisServidorDedicado Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    //ignora SIGINT - o servidor dedicado não deve terminar com Ctrl+C
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Erro ao ignorarrt o sinal SIGINT: %s", strerror(errno));
        exit(0);
    }

    //configura handler para SIGUSR2 - terminação ordenada pelo servidor principal
    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar o sinal SIGUSR2: %s", strerror(errno));
        exit(0);
    }

    //configura handler para SIGALRM - timeout no checkout
    if (signal(SIGALRM, sd10_1_1_TrataAlarme) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar o sinal SIGALRM: %s", strerror(errno));
        exit(0);
    }

    so_success("SD7.1", "Armei os sinais");

    so_debug(">");
}


/**
 * @brief sd7_2_ValidaPidCliente Ler a descrição da tarefa SD7.2 no enunciado
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    //vrifica se o PID é válido (positivo)
    if (clientRequest.msgData.est.pidCliente <= 0) {
        so_error("SD7.2", "PID do cliente inválido %d", clientRequest.msgData.est.pidCliente);
        exit(1);
    }

    so_success("SD7.2", "PID do cliente %d é válido", clientRequest.msgData.est.pidCliente);

    so_debug(">");
}

/**
 * @brief sd7_3_GetShmFACE Ler a descrição da tarefa SD7.3 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param pshmIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_3_GetShmFACE(key_t ipcKeyFace, int *pshmIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    //obtém o ID da memória compartilhada usando a chave fornecida
    *pshmIdFACE = shmget(ipcKeyFace, sizeof(int), 0);

    //verifica se houve erro na obtenção da memória compartilhada
    if (*pshmIdFACE == -1){
        so_error("SD7.3", "Erro ao obter o ID da memória compartilhada da FACE: %s", strerror(errno));
        exit(1);
    }

    //conecta à memória compartilhada e obtém ponteiro para a tarifa atual
    tarifaAtual = (int *)shmat(*pshmIdFACE, NULL, 0);
    
    //verifica se a conexão à memória compartilhada foi bem-sucedida
    if(tarifaAtual == (int *)-1){
        so_error("SD7.3", "Erro ao conectar à memória comapartilhada da FACE: %s", strerror(errno));
        exit(1);
    }

    so_success("SD7.3", "Conectado com sucesso ao SHM da FACE. ID: %d", *pshmIdFACE);

    so_debug("> [@return *pshmIdFACE:%d]", *pshmIdFACE);
}

/**
 * @brief sd7_4_GetSemFACE Ler a descrição da tarefa SD7.4 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param psemIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_4_GetSemFACE(key_t ipcKeyFace, int *psemIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    //obtém acesso ao grupo de semáforos existente da FACE
    *psemIdFACE = semget(ipcKeyFace, 0, 0);
    
    //verifica se conseguiu obter o grupo de semáforos
    if (*psemIdFACE == -1) {
        so_error("SD7.4", "Erro ao obter o ID do grupo de semáforos da FACE: %s", strerror(errno));
        exit(1);
    }

    so_success("SD7.4", "Conectado com sucesso ao grupo de semáforos da FACE. ID: %d", *psemIdFACE);

    so_debug("> [@return *psemIdFACE:%d]", *psemIdFACE);
}

/**
 
@brief sd7_5_ProcuraLugarDisponivelBD Ler a descrição da tarefa SD7.5 no enunciado,
@param semId (I) identificador aberto de IPC,
@param clientRequest (I) pedido recebido, enviado por um Cliente,
@param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD,
@param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador,
@param pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível*/

void sd7_5_ProcuraLugarDisponivelBD(int semId, MsgContent clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param semId:%d, clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", semId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);

    *pindexClienteBD = -1;

    //espera por vaga disponível
    struct sembuf semLugares = {SEM_LUGARES_PARQUE, -1, 0};
    if (semop(semId, &semLugares, 1) == -1) {
        so_error("SD7.5", "Erro ao esperar por vaga: %s", strerror(errno));
        return;
    }

    //bloqueia acesso à BD
    struct sembuf semBD = {SEM_MUTEX_BD, -1, 0};
    if (semop(semId, &semBD, 1) == -1) {
        so_error("SD7.5", "Erro ao bloquear BD: %s", strerror(errno));
        
        struct sembuf semLugaresRevert = {SEM_LUGARES_PARQUE, 1, 0};
        semop(semId, &semLugaresRevert, 1);
        
        return;
    }

    //procura lugar disponível
    *pindexClienteBD = -1;
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == DISPONIVEL) {
            lugaresEstacionamento[i] = clientRequest.msgData.est;
            *pindexClienteBD = i;
            so_success("SD7.5", "Reservei Lugar: %d", i);
            break;
        }
    }

    //libera BD (SEMPRE executar esta parte)
    semBD.sem_op = 1;
    if (semop(semId, &semBD, 1) == -1) {
        so_error("SD7.5", "Erro ao libertar BD: %s", strerror(errno));
    }


    so_debug("> [pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    const char *matricula = clientRequest.msgData.est.viatura.matricula;
    // verifica cada caracter da matrícula
    for (size_t i = 0; i < strlen(matricula); i++) {
        //se não for letra maiúscula nem dígito, matrícula é inválida
        if (!isupper(matricula[i]) && !isdigit(matricula[i])) {
            // Envia sinal SIGHUP para o cliente informando erro
            so_error("SD8.1", "Matricula inválida: %c. (A matricula deve ter somente letras maiúsculas e números)", matricula[i]);
            sd11_EncerraServidorDedicado();
            return;
        }
    }

    so_success("SD8.1", "Matricula %s validada com sucesso", clientRequest.msgData.est.viatura.matricula);
    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_2_ValidaPais(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    const char *pais = clientRequest.msgData.est.viatura.pais;

    //verifica se o código do país tem exatamente 2 caracteres
    if (strlen(pais) != 2) {
        kill(clientRequest.msgData.est.pidCliente, SIGHUP);
        so_error("SD8.2", "País inválido: %s (deve ter exatamente 2 caracteres)", pais);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    //verifica se ambos os caracteres são letras maiúsculas
    for (int i = 0; i < 2; i++) {
        if (!isupper(pais[i])) {
            kill(clientRequest.msgData.est.pidCliente, SIGHUP);
            so_error("SD8.2", "País inválido: %s (deve conter apenas letras maiúsculas)", pais);
            sd11_EncerraServidorDedicado();
            exit(1);
        }
    }

    so_success("SD8.2", "País válido: %s", clientRequest.msgData.est.viatura.pais);

    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    char categoria = clientRequest.msgData.est.viatura.categoria;
    //verifica se a categoria é uma das válidas: P, L, M
    if (categoria != 'P' && categoria != 'L' && categoria != 'M') {
        so_error("SD8.3", "Categoria inválida: %c (deve ser 'P', 'L' ou 'M')", categoria);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.3", "Categoria válida: %c", categoria);

    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    const char *nomeCondutor = clientRequest.msgData.est.viatura.nomeCondutor;   
    //abre o ficheiro /etc/passwd para verificar utilizadores do sistema
    FILE *file = fopen("/etc/passwd", "r");
    if (!file) {
        so_error("SD8.4", "Erro ao abrir o ficheiro dos nomes: %s", strerror(errno));
        sd11_EncerraServidorDedicado();
        
        return;
    }
    
    char line[256];
    int user_found = 0;

    //lê linha a linha o ficheiro /etc/passwd
    while (fgets(line, sizeof(line), file)) {
        char *f[7];
        int n = 0;
          //divide a linha pelos dois pontos (:)
        char *token = strtok(line, ":");

        //extrai os campos da linha (máximo 7 campos)
        while (token && n < 7) {
            f[n++] = token;
            token = strtok(NULL, ":");
        }

        // o 5º campo (índice 4) contém informações do utilizador incluindo o nome
        if (n >= 5) {
            char *nome = strtok(f[4], ",");
            if (nome && strcmp(nome, nomeCondutor) == 0) {
                user_found = 1;
                break;
            }
        }
    }
    fclose(file);

    //verifica se o utilizador foi encontrado
    if(user_found){
        so_success("SD8.4", "Nome do condutor válido: %s", clientRequest.msgData.est.viatura.nomeCondutor);
    }
    else {
        so_error("SD8.4", "Nome do condutor '%s' não encontrado na lista de utilizadores (/etc/passwd)", nomeCondutor);
        sd11_EncerraServidorDedicado();
        return;
    }


    so_debug(">");
}

/**
* @brief sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
*/
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    //inicializa gerador de números aleatórios com timestamp atual
    srand(time(NULL));

    // Gera tempo de espera aleatório entre 1 e MAX_ESPERA segundos
    int tempoEspera = (rand() % MAX_ESPERA) + 1;
    
    so_success("SD9.1", "Processo burocrático vai demorar %d segundos", tempoEspera);
    
    // Dorme pelo tempo especificado (simula processamento)
    sleep(tempoEspera);
    
    so_success("SD9.1", "Processo burocrático concluído após %d segundos", tempoEspera);

    so_debug(">");
}

/**
* @brief sd9_2_EnviaSucessoAoCliente Ler a descrição da tarefa SD9.2 no enunciado
* @param msgId (I) identificador aberto de IPC
* @param clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd9_2_EnviaSucessoAoCliente(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    MsgContent msgToClient;
    //configura o tipo da mensagem com o PID do cliente (para entrega direcionada)
    msgToClient.msgType = clientRequest.msgData.est.pidCliente;
    
    //copia informações do cliente para a resposta
    msgToClient.msgData.est.pidCliente = clientRequest.msgData.est.pidCliente;
    msgToClient.msgData.est.pidServidorDedicado = clientRequest.msgData.est.pidServidorDedicado;
    
    //define status como aceite
    msgToClient.msgData.status = CLIENT_ACCEPTED;

    //envia a mensagem através da fila de mensagens
    if (msgsnd(msgId, &msgToClient, sizeof(MsgContent) - sizeof(long), 0) == -1) {
        so_error("SD9.2", "Erro ao enviar mensagem de sucesso ao cliente: %s", strerror(errno));
        sd11_EncerraServidorDedicado();
        return;
    }
    
    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);

    so_debug(">");
}

/**
* @brief sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
* @param logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
* @param clientRequest (I) pedido recebido, enviado por um Cliente
* @param pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
* @param plogItem (O) registo de Log para esta viatura
*/
void sd9_3_EscreveLogEntradaViatura(char *logFilename, MsgContent clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]", logFilename, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    //abre o ficheiro de log em modo append binário (adiciona ao final)
    FILE *ficheiroLog = fopen(logFilename, "ab+");
    if (!ficheiroLog) {
        so_error("SD9.3", "Erro ao abrir o ficheiro de logs %s", logFilename);
        sd11_EncerraServidorDedicado();
        exit(0);
    }

    //posiciona-se no final do ficheiro
    if (fseek(ficheiroLog, 0, SEEK_END) != 0) {
        so_error("SD9.3", "Erro ao posicionar no final do ficheiro de logs");
        fclose(ficheiroLog);
        sd11_EncerraServidorDedicado();
        exit(0);
    }

     //obtém a posição atual (final do ficheiro) antes de escrever
    long posicaoFinal = ftell(ficheiroLog);
    if (posicaoFinal == -1) {
        so_error("SD9.3", "Erro ao obter posição no ficheiro de logs");
        fclose(ficheiroLog);
        sd11_EncerraServidorDedicado();
        exit(0);
    }
    *pposicaoLogfile = posicaoFinal;

    //cria o registo de log com informações da viatura
    *plogItem = (LogItem){
        .viatura = clientRequest.msgData.est.viatura
    };

    //obtém timestamp atual e formata para string
    time_t instanteAtual = time(NULL);
    struct tm *dataFormatada = localtime(&instanteAtual);
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", dataFormatada);
    plogItem->dataSaida[0] = '\0';

    //escreve o registo no ficheiro
    if (fwrite(plogItem, sizeof(LogItem), 1, ficheiroLog) != 1) {
        so_error("SD9.3", "Erro ao escrever log de entrada");
        fclose(ficheiroLog);
        sd11_EncerraServidorDedicado();
        exit(0);
    }

    fclose(ficheiroLog);
    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s",*pposicaoLogfile,plogItem->viatura.matricula,plogItem->dataEntrada);
    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}

/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 */
void sd10_1_AguardaCheckout(int msgId) {
    so_debug("< [@param msgId:%d]", msgId);
    MsgContent msg;
    int receivedTermina = 0;
    alarm(60);
    while (!receivedTermina) {
        ssize_t bytesRead = msgrcv(msgId, &msg, sizeof(msg.msgData), getpid(), 0);
        if (bytesRead == -1) {
            if (errno == EINTR) {
                continue;
            }
            so_error("SD10.1", "Erro ao receber mensagem de checkout: %s", strerror(errno));
            sd11_EncerraServidorDedicado();
            return;
        }
        
        if (msg.msgData.status == TERMINA_ESTACIONAMENTO) {
            clientRequest = msg;
            so_success("SD10.1", "SD: A viatura %s deseja sair do parque", msg.msgData.est.viatura.matricula);
            receivedTermina = 1;
        } else {

            so_debug("SD10.1", "Received unexpected message type: %d", msg.msgData.status);
        }
    }
    alarm(0);

    so_debug(">");
}

/**
 * @brief  sd10_1_1_TrataAlarme Ler a descrição da tarefa SD10.1.1 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd10_1_1_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    time_t tempo_atual;
    struct tm *info_tempo;
    char data_hora[64];
    char texto_completo[256];
    int tarifa_valor = 0;
    struct sembuf semOp;
    
    // PROTEÇÃO CONTRA DEADLOCK - Usar semáforo para SHM FACE
    semOp.sem_num = SEM_MUTEX_FACE; //constante específica para mutex FACE
    semOp.sem_op = -1;  //operação P (wait)
    semOp.sem_flg = IPC_NOWAIT;  // CRÍTICO: Non-blocking para evitar deadlock
    
    //tentar entrar na zona crítica sem bloquear
    if (semop(semIdFACE, &semOp, 1) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            //semáforo ocupado - evitar deadlock reagendando sem bloquear
            so_debug("SD10.1.1: Semáforo ocupado, reagendando alarme");
            alarm(60);
            so_debug(">");
            return;
        } else {
            so_error("SD10.1.1", "Erro ao adquirir semáforo FACE: %s", strerror(errno));
            alarm(60);
            so_debug(">");
            return;
        }
    }
    
    // INÍCIO DA ZONA CRÍTICA - Protegida contra deadlock
    
    //ler tarifa da SHM FACE (acesso à memória partilhada protegido)
    if (tarifaAtual != NULL && tarifaAtual != (int*)-1) {
        tarifa_valor = *tarifaAtual;
    } else {
        // SAIR DA ZONA CRÍTICA em caso de erro
        semOp.sem_op = 1;
        semOp.sem_flg = 0;
        semop(semIdFACE, &semOp, 1);
        so_error("SD10.1.1", "Ponteiro tarifaAtual não está válido");
        alarm(60);
        so_debug(">");
        return;
    }
    
    //obter data e hora atual no formato correto
    time(&tempo_atual);
    info_tempo = localtime(&tempo_atual);
    strftime(data_hora, sizeof(data_hora), "%Y-%m-%dT%Hh%M", info_tempo);
    
    //construir mensagem com formato esperado
    snprintf(texto_completo, sizeof(texto_completo), 
             "%s Tarifa atual:%d", data_hora, tarifa_valor);
    
    // FIM DA ZONA CRÍTICA - Liberar semáforo ANTES de enviar mensagem
    semOp.sem_op = 1;
    semOp.sem_flg = 0;
    if (semop(semIdFACE, &semOp, 1) == -1) {
        so_error("SD10.1.1", "Erro ao liberar semáforo FACE: %s", strerror(errno));
    }
    
    //preparar mensagem ao Cliente
    MsgContent msgToClient;
    msgToClient.msgType = clientRequest.msgData.est.pidCliente;
    msgToClient.msgData.status = INFO_TARIFA;
    msgToClient.msgData.est.pidCliente = clientRequest.msgData.est.pidCliente;
    msgToClient.msgData.est.pidServidorDedicado = getpid();
    strncpy(msgToClient.msgData.infoTarifa, texto_completo, 
            sizeof(msgToClient.msgData.infoTarifa) - 1);
    msgToClient.msgData.infoTarifa[sizeof(msgToClient.msgData.infoTarifa) - 1] = '\0';
    
    //enviar mensagem APÓS sair da zona crítica (evita deadlock na comunicação)
    if (msgsnd(msgId, &msgToClient, sizeof(msgToClient) - sizeof(long), IPC_NOWAIT) == 0) {
        so_success("SD10.1.1", "Info Tarifa");
    } else {
        so_error("SD10.1.1", "Erro ao enviar mensagem de tarifa ao cliente: %s", strerror(errno));
    }
    
    //reagendar o próximo alarme para 1 minuto (60 segundos)
    alarm(60);
    
    so_debug(">");
}

/**
 * @brief  sd10_2_EscreveLogSaidaViatura Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 */
void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]", logFilename, posicaoLogfile, logItem.viatura.matricula, logItem.viatura.pais, logItem.viatura.categoria, logItem.viatura.nomeCondutor, logItem.dataEntrada, logItem.dataSaida);

    //abertura do ficheiro em modo read/write binário
    FILE *fp = fopen(logFilename, "rb+");
    if (!fp) {
        so_error("SD10.2", "Erro ao abrir o ficheiro de logs %s: %s", logFilename, strerror(errno));
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    //posicionamento no ficheiro
    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) {
        so_error("SD10.2", "Erro ao posicionar no ficheiro de logs na posição %ld: %s", posicaoLogfile, strerror(errno));
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    //geração e validação do timestamp de saída
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        so_error("SD10.2", "Erro ao obter timestamp atual");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    struct tm *tm_info = localtime(&now);
    if (!tm_info) {
        so_error("SD10.2", "Erro ao converter timestamp para hora local");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    //formatação da data de saída
    size_t strftime_result = strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", tm_info);
    if (strftime_result == 0) {
        so_error("SD10.2", "Erro ao formatar data de saída");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    //escrita do registo atualizado
    size_t bytes_written = fwrite(&logItem, sizeof(LogItem), 1, fp);
    if (bytes_written != 1) {
        so_error("SD10.2", "Erro ao atualizar log da saída do condutor: %s", strerror(errno));
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    //fechar o ficheiro
    if (fclose(fp) != 0) {
        so_error("SD10.2", "Erro ao fechar ficheiro de logs: %s", strerror(errno));
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    //sucesso na operação
    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s", posicaoLogfile, logItem.viatura.matricula, logItem.dataSaida);

    //encerra o servidor dedicado após operação bem-sucedida
    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(semId, lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaTerminarAoClienteETermina(msgId, clientRequest);

    so_debug(">");
}

/**
 * @brief sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(int semId, Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param semId:%d, lugaresEstacionamento:%p, indexClienteBD:%d]", semId, lugaresEstacionamento, indexClienteBD);


    //verificação do índice
    if (indexClienteBD < 0 ) {
        so_error("SD11.1", "Índice inválido (%d)", indexClienteBD);
        return;
    }

    // 1ª operação: Lock do mutex (SEM_MUTEX_BD = -1)
    struct sembuf op1;
    op1.sem_num = SEM_MUTEX_BD;  
    op1.sem_op = -1;
    op1.sem_flg = 0;
    if (semop(semId, &op1, 1) == -1) {
        so_error("SD11.1", "Erro ao bloquear mutex");
        return;
    }

    //libertar o lugar no estacionamento
    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    
    //mensagem de sucesso
    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);

    // 2ª operação: Incrementar semáforo de lugares (SEM_LUGARES_PARQUE = +1)
    struct sembuf op2;
    op2.sem_num = SEM_LUGARES_PARQUE;  //usar a constante definida no sistema
    op2.sem_op = 1;
    op2.sem_flg = 0;
    if (semop(semId, &op2, 1) == -1) {
        so_error("SD11.1", "Erro ao incrementar lugares");
        return;
    }

    // 3ª operação: Unlock do mutex (SEM_MUTEX_BD = +1)
    struct sembuf op3;
    op3.sem_num = SEM_MUTEX_BD;
    op3.sem_op = 1;
    op3.sem_flg = 0;
    if (semop(semId, &op3, 1) == -1) {
        so_error("SD11.1", "Erro ao desbloquear mutex");
        return;
    }


    so_debug(">");
}

/**
 * @brief sd11_2_EnviaTerminarAoClienteETermina Ler a descrição da tarefa SD11.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaTerminarAoClienteETermina(int msgId, MsgContent clientRequest) {
    so_debug("<");
    //termino do cliente
    MsgContent response;
    response.msgType = clientRequest.msgData.est.pidCliente;  
    response.msgData.status = ESTACIONAMENTO_TERMINADO;
    response.msgData.est = clientRequest.msgData.est;

    if (msgsnd(msgId, &response, sizeof(response.msgData), 0) == -1) {
        so_error("SD11.2", "Erro ao enviar mensagem ao cliente");
    } else {
        so_success("SD11.2", "SD: Shutdown");
    }

    exit(0);
    

    so_debug(">");
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD12", "SD: Recebi pedido do Servidor para terminar");

    struct sembuf semOp;
    semOp.sem_num = SEM_SRV_DEDICADOS;
    semOp.sem_op = SEM_UP;
    semOp.sem_flg = 0;
    semop(semId, &semOp, 1);

    sd11_EncerraServidorDedicado();


    so_debug(">");

}