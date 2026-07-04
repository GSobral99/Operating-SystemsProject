/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº: 129850      Nome: Gonçalo Sobral
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo: O Módulo Cliente permite aos utilizadores realizar check-in de viaturas
**no sistema de estacionamento, comunicando com o servidor através de Message Queues
++do System V. O funcionamento inicia-se com a conexão à message queue partilhada e configuração
**de handlers para os sinais SIGINT (Ctrl+C) e SIGALRM (timeout). Durante a fase de check-in,
**o cliente recolhe os dados da viatura (matrícula, país, categoria e nome do condutor) através
**de uma interface interativa, validando que todos os campos estão devidamente preenchidos antes
**de enviar o pedido ao servidor principal. Após o envio, programa um alarme para evitar espera
**infinita e aguarda a confirmação do servidor, cancelando o timeout quando recebe resposta.
**Uma vez aceite, entra num loop principal onde recebe continuamente mensagens do servidor
**dedicado, apresentando atualizações de tarifa ao utilizador e terminando quando recebe o sinal
**de fim de estacionamento. O módulo distingue entre dois estados principais: pré check-in, onde
**apenas aguarda confirmação, e pós check-in, onde pode terminar voluntariamente através do
**Ctrl+C, enviando uma mensagem de término ao servidor dedicado.
*/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int msgId = -1;                         // Variável que tem o ID da Message Queue
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief Processamento do processo Cliente.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_GetMsgQueue(IPC_KEY, &msgId);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    c2_2_EscrevePedido(msgId, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor(msgId, &clientRequest);
    c4_2_DesligaAlarme();

    c5_MainCliente(msgId, &clientRequest);

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief c1_1_GetMsgQueue Ler a descrição da tarefa C1.1 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void c1_1_GetMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    //conecta a um queue existente
    *pmsgId = msgget(ipcKey, IPC_GET);
    if (*pmsgId < 0) {
        so_error("C1.1", "Erro ao abrir a Message Queue");
        exit(1);
    }

    so_success("C1.1", "Message Queue aberta com sucesso");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.2 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");

    //configura o SIGINT
    if (signal(SIGINT, c6_TrataCtrlC) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar o sinal SIGINT");
        exit(1);
    }

    //configura o SIGALARM
    if (signal(SIGALRM, c7_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar o sinal SIGALRM");
        exit(1);
    }

    so_success("C1.2", "Sinais armados com sucesso");

    so_debug(">");
}

/**
 * @brief c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(MsgContent *pclientRequest) {
    so_debug("<");

    // interface do user
    printf("Park-IUL: Check-in Viatura\n");
    printf("----------------------------\n");


    int isValid = 0;//flag para a validação de input
    char buffer[256]; //buffer temporário

    // lê matricula da viatura com validação
    do {
        printf("Introduza a matrícula da viatura: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            so_error("C2.1", "Erro ao ler a matrícula");
            exit(1);
        }
        buffer[strcspn(buffer, "\n")] = 0; //remove a nova linha

        //verificar que não há espaços
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (!isspace((unsigned char)buffer[i])) {
                isValid = 1;
                break;
            }
        }
    } while (!isValid);//repetir até ser um input válido
    strcpy(pclientRequest->msgData.est.viatura.matricula, buffer);

    // lê país da viatura   
    do {
        printf("Introduza o país da viatura: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            so_error("C2.1", "Erro ao ler o país");
            exit(1);
        }
        buffer[strcspn(buffer, "\n")] = 0; 
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (!isspace((unsigned char)buffer[i])) {
                isValid = 1;
                break;
            }
        }
    } while (!isValid);
    strcpy(pclientRequest->msgData.est.viatura.pais, buffer);

    //lê categoria
    do {
        printf("Introduza a categoria da viatura: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            so_error("C2.1", "Erro ao ler a categoria");
            exit(1);
        }
        buffer[strcspn(buffer, "\n")] = 0; 
        
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (!isspace((unsigned char)buffer[i])) {
                isValid = 1;
                break;
            }
        }
    } while (!isValid);
    pclientRequest->msgData.est.viatura.categoria = buffer[0];

    // lê nome do condutor
    do {
        printf("Introduza o nome do condutor: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            so_error("C2.1", "Erro ao ler o nome do condutor");
            exit(1);
        }
        buffer[strcspn(buffer, "\n")] = 0; 
        
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (!isspace((unsigned char)buffer[i])) {
                isValid = 1;
                break;
            }
        }
    } while (!isValid);
    strcpy(pclientRequest->msgData.est.viatura.nomeCondutor, buffer);

    // preencher campos
    pclientRequest->msgData.est.pidCliente = getpid();
    pclientRequest->msgData.est.pidServidorDedicado = -1;

    //log dos dados coletados
    so_success("C2.1", "%s %s %c %s %d %d", 
        pclientRequest->msgData.est.viatura.matricula, 
        pclientRequest->msgData.est.viatura.pais, 
        pclientRequest->msgData.est.viatura.categoria, 
        pclientRequest->msgData.est.viatura.nomeCondutor, 
        pclientRequest->msgData.est.pidCliente, 
        pclientRequest->msgData.est.pidServidorDedicado);

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief c2_2_EscrevePedido Ler a descrição da tarefa C2.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_2_EscrevePedido(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);
    //definier tipo de msg
    clientRequest.msgType = MSGTYPE_LOGIN;

    // envia mensagem para o server
    if (msgsnd(msgId, &clientRequest, sizeof(clientRequest.msgData), NO_FLAGS) < 0) {
        so_error("C2.2", "Erro ao enviar mensagem para o Servidor");
        exit(1);
    }

    so_success("C2.2", "Pedido enviado com sucesso");


    so_debug(">");
}

/**
 * @brief c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);

    //programa alarm que vai gerar o SIGALARM
    alarm(segundos);

    so_success("C3", "Espera resposta em %d segundos", segundos);

    so_debug(">");
}

/**
 * @brief c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c4_1_EsperaRespostaServidor(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    //recebe mensagem direcionada especificamente a este cliente (getpid())
    //bloqueia até receber mensagem com msgType == PID do cliente
    if (msgrcv(msgId, pclientRequest, sizeof(pclientRequest->msgData), getpid(), NO_FLAGS) < 0) {
        so_error("C4.1", "Erro ao receber mensagem do Servidor");
        exit(1);
    }

    //verifica o status da resposta
    if (pclientRequest->msgData.status == CLIENT_ACCEPTED) {
        //cliente aceite - pode estacionar
        so_success("C4.1", "Check-in realizado com sucesso");
        recebeuRespostaServidor = TRUE;
    } else if (pclientRequest->msgData.status == ESTACIONAMENTO_TERMINADO) {
        //estacionamento está cheio ou não disponível
        so_success("C4.1", "Não é possível estacionar");
        exit(0);
    }

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief c4_2_DesligaAlarme Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    // cancela alarm anterior
    alarm(0);

    so_success("C4.2", "Desliguei alarme");

    so_debug(">");
}

/**
 * @brief c5_MainCliente Ler a descrição da tarefa C5 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c5_MainCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    //loop para receber mensagens do servidor dedicado
    while (1) {
        //aguarda mensagens direcionadas ao PID deste cliente  
        if (msgrcv(msgId, pclientRequest, sizeof(pclientRequest->msgData), getpid(), NO_FLAGS) < 0) {
            so_error("C5", "Erro ao receber mensagem do Servidor Dedicado");
            exit(1);
        }
        
        //processa diferentes tipos de mensagem
        if (pclientRequest->msgData.status == INFO_TARIFA) {
            // Recebeu atualização de tarifa - mostra ao user
            so_success("C5", "%s", pclientRequest->msgData.infoTarifa);
        } else if (pclientRequest->msgData.status == ESTACIONAMENTO_TERMINADO) {
            //server indica o fim do estacionamento
            so_success("C5", "Estacionamento terminado"); 
            exit(0);
        }
    }

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief  c6_TrataCtrlC Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d, msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", sinalRecebido, msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    //s´´o permite terminar se já fez check-in
    if (recebeuRespostaServidor){
        //prepara mensagem de término para o servidor dedicado
        clientRequest.msgType = clientRequest.msgData.est.pidServidorDedicado;
        clientRequest.msgData.status = TERMINA_ESTACIONAMENTO;
        //envia mensagem de t´rmino
        if (msgsnd(msgId, &clientRequest, sizeof(clientRequest.msgData), NO_FLAGS) < 0) {
            so_error("C6", "Erro ao enviar mensagem de término");
            exit(1);
        }
        so_success("C6", "Cliente: Shutdown");
    }else{
        so_error("C6", "Ainda não foi feito check-in");
        exit(1);
    }


    so_debug(">");
}

/**
 * @brief  c7_TrataAlarme Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    //timeout se o servidor não responder a tempo
    so_error("C7", "Cliente: Timeout");
    exit(0);

    so_debug(">");
}