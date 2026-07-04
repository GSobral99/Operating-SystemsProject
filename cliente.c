/****************************************************************************************
** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 4+
**
** Aluno: Nº: 129850        Nome: Gonçalo Sobral
** Nome do Módulo: cliente.c
** Descrição/Explicação do Módulo: Este módulo implementa a funcionalidade de um Cliente para um sistema de estacionamento.
** O Cliente é responsável por:
**   1. Validar a conexão com o Servidor através de um FIFO
**   2. Configurar tratadores de sinais para comunicação interprocessos
**   3. Recolher informações sobre a viatura a estacionar (matrícula, país, categoria, condutor)
**   4. Enviar pedidos de estacionamento ao Servidor através do FIFO
**   5. Aguardar resposta do Servidor dedicado com timeout programável
**   6. Gerir o processo de check-in e checkout do estacionamento
**   7. Encerrar a conexão de forma controlada, notificando o Servidor
**
** O módulo implementa um protocolo de comunicação baseado em sinais (SIGUSR1, SIGHUP, SIGINT, SIGALRM)
** para coordenar com o Servidor o processo de estacionamento, desde a entrada até a saída do veículo.
**
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief  Processamento do processo Cliente.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_ValidaFifoServidor(FILE_REQUESTS);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    FILE *fFifoServidor;
    c2_2_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
    c2_3_EscrevePedido(fFifoServidor, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor();
    c4_2_DesligaAlarme();
    c4_3_InputEsperaCheckout();

    c5_EncerraCliente();

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    return 0;
    so_debug(">");
}

/**
 * @brief  c1_1_ValidaFifoServidor Ler a descrição da tarefa C1.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void c1_1_ValidaFifoServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    // Verifica se o arquivo FIFO existe
    if (access(filenameFifoServidor, F_OK) == -1) {
        so_error("C1.1", "O FIFO do servidor não existe");
        exit(1);
    }

    struct stat st;//estrutura para armazenar informações sobre o arquivo
    if (stat(filenameFifoServidor, &st) == -1) {// obtem informações do arquivo
        so_error("C1.1", "Erro ao verificar o tipo do arquivo");
        exit(1);
    }
    
    if (!S_ISFIFO(st.st_mode)) {//verifica se o arquivo é realmente um FIFO/pipe
        so_error("C1.1", "O ficheiro existe, mas não é um FIFO");
        exit(1);
    }
    
    so_success("C1.1", "FIFO do servidor validado com sucesso");

    so_debug(">");
}

/**
 * @brief  c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.3 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");

    struct sigaction sa_usr1;// Estrutura para configurar ação para o sinal SIGUSR1
    sa_usr1.sa_sigaction = c6_TrataSigusr1;//define função que tratará o sinal SIGUSR1
    sa_usr1.sa_flags = SA_SIGINFO; // Configura flags para usar sa_sigaction e obter informações adicionais
    sigemptyset(&sa_usr1.sa_mask); // Inicializa a msscara de sinais vazia
    
    // Registra o tratador para SIGUSR1
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        so_error("C1.2", "Erro ao armar o sinal SIGUSR1");
        exit(1);
    }

    // Registra o tratador para SIGHUP
    if (signal(SIGHUP, c7_TrataSighup) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar o sinal SIGHUP");
        exit(1);
    }

    // Registra o tratador para SIGINT (Ctrl+C)
    if (signal(SIGINT, c8_TrataCtrlC) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar o sinal SIGINT");
        exit(1);
    }

    // Registra o tratador para SIGALRM (alarme)
    if (signal(SIGALRM, c9_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar o sinal SIGALRM");
        exit(1);
    }
    
    so_success("C1.2", "Sinais armados com sucesso");

    so_debug(">");
}

/**
 * @brief  c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param  pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(Estacionamento *pclientRequest) {
    so_debug("<");

    // Variáveis para validação
    int valido = FALSE;
    char input[80];
    
    // Preenchimento da matrícula
    do {
        printf("Introduza a matrícula do veículo: ");
        scanf("%9s", input);
        if (strlen(input) > 0) {
            strcpy(pclientRequest->viatura.matricula, input);
            valido = TRUE;
        } else {
            printf("Matrícula inválida. Tente novamente.\n");
        }
    } while (!valido);
    
    // Preenchimento do país
    valido = FALSE;
    do {
        printf("Introduza o país (código de 2 letras): ");
        scanf("%2s", input);
        if (strlen(input) > 0) {
            strcpy(pclientRequest->viatura.pais, input);
            valido = TRUE;
        } else {
            printf("País inválido. Tente novamente.\n");
        }
    } while (!valido);
    
    // Preenchimento da categoria
    valido = FALSE;
    do {
        printf("Introduza a categoria do veículo (um caractere): ");
        // Limpa o buffer antes de ler o próximo caractere
        getchar();
        pclientRequest->viatura.categoria = getchar();
        if (pclientRequest->viatura.categoria != ' ' && pclientRequest->viatura.categoria != '\n') {
            valido = TRUE;
        } else {
            printf("Categoria inválida. Tente novamente.\n");
        }
    } while (!valido);
    
    // Preenchimento do nome do condutor
    valido = FALSE;
    do {
        printf("Introduza o nome do condutor: ");
        // Limpa o buffer antes de ler o nome
        getchar();
        fgets(input, 79, stdin);
        // Remove o caractere de nova linha, se existir
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }
        if (strlen(input) > 0) {
            strcpy(pclientRequest->viatura.nomeCondutor, input);
            valido = TRUE;
        } else {
            printf("Nome inválido. Tente novamente.\n");
        }
    } while (!valido);
    
    // Preenche o PID do cliente e inicializa o PID do servidor dedicado como -1
    pclientRequest->pidCliente = getpid();
    pclientRequest->pidServidorDedicado = -1;

    so_success("C2.1", "%s %s %c %s %d %d",
               pclientRequest->viatura.matricula,
               pclientRequest->viatura.pais,
               pclientRequest->viatura.categoria,
               pclientRequest->viatura.nomeCondutor,
               pclientRequest->pidCliente,
               pclientRequest->pidServidorDedicado);
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->viatura.matricula, pclientRequest->viatura.pais, pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
}

/**
 * @brief  c2_2_AbreFifoServidor Ler a descrição da tarefa C2.2 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 */
void c2_2_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    *pfFifoServidor = fopen(filenameFifoServidor, "w"); // Abre o FIFO para escrita
    if (*pfFifoServidor == NULL) {// Verifica se houve erro ao abrir o FIFO
        so_error("C2.2", "Erro ao abrir FIFO do servidor");
        exit(1);
    }

    so_success("C2.2", "FIFO do servidor aberto com sucesso");

    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
 * @brief  c2_3_EscrevePedido Ler a descrição da tarefa C2.3 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_3_EscrevePedido(FILE *fFifoServidor, Estacionamento clientRequest) {
    so_debug("< [@param fFifoServidor:%p, clientRequest:[%s:%s:%c:%s:%d:%d]]", fFifoServidor, clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (fwrite(&clientRequest, sizeof(Estacionamento), 1, fFifoServidor) != 1) {// Escreve a estrutura no FIFO e verifica se escreveu 1 item
        so_error("C2.3", "Erro ao escrever no FIFO");
        exit(1);
    }


    fclose(fFifoServidor);
    so_success("C2.3", "Pedido escrito com sucesso no FIFO");

    so_debug(">");
}

/**
 * @brief  c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param  segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);

    alarm(segundos);//alarme para disparar após o número de segundos

    so_success("C3", "Espera resposta em %d segundos", segundos);

    so_debug(">");
}

/**
 * @brief  c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4 no enunciado
 */
void c4_1_EsperaRespostaServidor() {
    so_debug("<");

    pause();   // Suspende o processo até que um sinal seja recebido
    
    so_success("C4.1", "Check-in realizado com sucesso");
    
    

    so_debug(">");
}

/**
 * @brief  c4_2_DesligaAlarme Ler a descrição da tarefa C4.1 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    alarm(0); // Desliga o alarme

    so_success("C4.2", "Desliguei alarme");

    so_debug(">");
}

/**
 * @brief  c4_3_InputEsperaCheckout Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_3_InputEsperaCheckout() {
    so_debug("<");
    char input[20]; //buffer para armazenar a entrada do utilizador
    do {
        printf("Digite 'sair' para terminar o estacionamento: ");// Solicita ao utilizador que digite 'sair'
        so_gets(input, sizeof(input)); //lê a entrada do utilizador
    } while (strcmp(input, "sair") != 0); // Repete até que o utilizador digite 'sair'

    so_success("C4.3", "Utilizador pretende terminar estacionamento");
    c5_EncerraCliente();

    so_debug(">");
}

/**
 * @brief  c5_EncerraCliente      Ler a descrição da tarefa C5 no enunciado
 */
void c5_EncerraCliente() {
    so_debug("<");

    c5_1_EnviaSigusr1AoServidor(clientRequest); // Envia sinal SIGUSR1 ao servidor dedicado
    c5_2_EsperaRespostaServidorETermina();// Espera resposta do servidor e termina

    so_debug(">");
}

/**
 * @brief  c5_1_EnviaSigusr1AoServidor      Ler a descrição da tarefa C5.1 no enunciado
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c5_1_EnviaSigusr1AoServidor(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    // Verifica se o PID do servidor dedicado é válido
    if (clientRequest.pidServidorDedicado <= 0) {
        so_error("C5.1", "PID do Servidor Dedicado inválido");
        exit(1);
    }

    // Envia o sinal SIGUSR1 ao processo do servidor dedicado
    if (kill(clientRequest.pidServidorDedicado, SIGUSR1) == -1) {
        so_error("C5.1", "Erro ao enviar SIGUSR1 ao servidor dedicado");
        exit(1);
    }

    so_success("C5.1", "Enviei SIGUSR1 ao servidor dedicado %d", clientRequest.pidServidorDedicado);
    so_debug(">");
}

/**
 * @brief  c5_2_EsperaRespostaServidorETermina      Ler a descrição da tarefa C5.2 no enunciado
 */
void c5_2_EsperaRespostaServidorETermina() {
    so_debug("<");

    pause();// Suspende o processo até que um sinal seja recebido

    so_success("C5.2", "Terminou");
    exit(0);

    so_debug(">");
}

/**
 * @brief  c6_TrataSigusr1      Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataSigusr1(int sinalRecebido, siginfo_t *siginfo, void *context) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    
    // Verifica se o sinal recebido é SIGUSR1
    if (sinalRecebido == SIGUSR1) {
        clientRequest.pidServidorDedicado = siginfo->si_pid;// Obtém o PID do processo que enviou o sinal
        recebeuRespostaServidor = TRUE;  // Marca que recebeu resposta do servidor
        
        so_success("C6", "Check-in concluído com sucesso pelo Servidor Dedicado %d", clientRequest.pidServidorDedicado);
    } else {
        so_error("C6", "Sinal recebido não é SIGUSR1");
    }

    so_debug(">");
}

/**
 * @brief  c7_TrataSighup      Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataSighup(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("C7", "Estacionamento terminado");// Exibe mensagem de sucesso
    exit(0);

    so_debug(">");
}

/**
 * @brief  c8_TrataCtrlC      Ler a descrição da tarefa c8 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c8_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("C8", "Cliente: Shutdown");
    c5_EncerraCliente(); // Chama a função para encerrar o cliente
    exit(0);

    so_debug(">");
}

/**
 * @brief  c9_TrataAlarme      Ler a descrição da tarefa c9 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c9_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C9", "Cliente: Timeout"); // Exibe mensagem de erro (timeout)
    exit(0);

    so_debug(">");
}
