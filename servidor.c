/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 2+
 **
 ** Aluno: Nº:129850    Nome: Gonçalo Sobral
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo: Este módulo implementa um servidor de gestão de estacionamento que processa pedidos de
** clientes para entrada e saída de veículos. O servidor principal recebe solicitações através 
** de um FIFO, verifica a disponibilidade de vagas, e cria servidores dedicados (SD) 
** para cada cliente aceite. Os SDs validam os dados dos veículos, autenticam os condutores 
** e gerenciam todo o ciclo de vida do estacionamento, incluindo o registo 
** de entradas e saídas em um arquivo de log. O sistema utiliza comunicação por sinais entre 
** processos para coordenar as operações, com tratamento específico para encerramento
** controlado e limpeza dos recursos alocados.
 **
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento;  // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD;                     // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile;                    // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile


/**
* @brief Processamento do processo Servidor e dos processos Servidor Dedicado
*/
int main (int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv);
    s2_MainServidor();
        
    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    return 0; 
    so_debug(">");
}

/**
* @brief  s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
*         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
* @param  argc (I) número de Strings do array argv
* @param  argv (I) array de lugares de estacionamento que irá servir de BD
*/
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");


    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_CriaBD(dimensaoMaximaParque, &lugaresEstacionamento);
    s1_3_ArmaSinaisServidor();
    s1_4_CriaFifoServidor(FILE_REQUESTS);

    so_debug(">");
    }

/**
* @brief  s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
* @param  argc (I) número de Strings do array argv
* @param  argv (I) array de lugares de estacionamento que irá servir de BD
* @param  pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
*/
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if (argc != 2) { //verificar se o nº de argumentos é 2
        so_error("S1.1", "Número incorreto de argumentos. Uso: %s <dimensaoMaximaParque>", argv[0]);
        exit(1); //terminar se der erro
    }

    // Converter o argumento para inteiro
    int dimensao = atoi(argv[1]);

    // Verificar se a dimensão é válida (maior que zero)
    if (dimensao <= 0) {
        so_error("S1.1", "A dimensão do parque deve ser um número inteiro positivo");
        exit(1); //terminar se for menor que 0
    }

    // Armazenar o valor no ponteiro fornecido
    *pdimensaoMaximaParque = dimensao;

    so_success("S1.1", "Dimensão do parque definida como %d", dimensao);

    so_debug("> [@param +pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
* @brief  s1_2_CriaBD      Ler a descrição da tarefa S1.2 no enunciado
* @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
* @param  plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
*/
void s1_2_CriaBD(int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    int i; //variavel para o for
    so_debug("< [@param dimensaoMaximaParque:%d]", dimensaoMaximaParque);
    	
    //alocar memória para o array de lugares
    *plugaresEstacionamento = (Estacionamento *) malloc(dimensaoMaximaParque * sizeof(Estacionamento));
    if (*plugaresEstacionamento == NULL) { //verificar se a alocação foi bem sucedida
        so_error("S1.2", "Erro na alocação de memória para o parque");
        exit(1); //termina se falhar
    }

    //iniciar os lugares todos como disponiveis /sem cliente
    for (i = 0; i < dimensaoMaximaParque; i++) {
        (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL; //pid do cliente como diposnivel
        (*plugaresEstacionamento)[i].pidServidorDedicado = DISPONIVEL; //pid do SD como disponivel    
    }

    so_success("S1.2", "Base de dados do parque criada com sucesso");

    so_debug("> [*plugaresEstacionamento:%p]", *plugaresEstacionamento);

}

/**
* @brief  s1_3_ArmaSinaisServidor Ler a descrição da tarefa S1.3 no enunciado
*/
void s1_3_ArmaSinaisServidor() {
    so_debug("<");

    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {//configura o sinal SIGINT CTRL+C
        so_error("S1.3", "Erro ao armar o sinal SIGINT");
        exit(1);
    }
        
    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) { //configura o SIGCHLD, termino do processo filho
        so_error("S1.3", "Erro ao armar o sinal SIGCHLD");
        exit(1);
    }
            
    so_success("S1.3", "Sinais do servidor armados com sucesso");

    so_debug(">");
}

/**
* @brief  s1_4_CriaFifoServidor Ler a descrição da tarefa S1.4 no enunciado
* @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
*/
void s1_4_CriaFifoServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    //remove o ficheiro FIFO se já existir
    unlink(filenameFifoServidor);
            
    //cria o FIFO com premissões de leitura e escrita
    if (mkfifo(filenameFifoServidor, 0600) == -1) {
        so_error("S1.4", "Erro ao criar o FIFO do servidor");
        exit(1);
    }
            
    so_success("S1.4", "FIFO do servidor criado com sucesso");

    so_debug(">");

}

/**
* @brief  s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
*         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO, exceto depois de
*         realizada a função s2_1_AbreFifoServidor(), altura em que podem
*         comentar o statement sleep abaixo (que, neste momento está aqui
*         para evitar que os alunos tenham uma espera ativa no seu código)
*/
void s2_MainServidor() {
    FILE *fFifoServidor;
    so_debug("<");

    while (TRUE) { 
        s2_1_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
        s2_2_LePedidosFifoServidor(fFifoServidor);
        //sleep(10);  // TEMPORÁRIO, os alunos deverão comentar este statement apenas
        //                     // depois de terem a certeza que não terão uma espera ativa
                
    }

    so_debug(">");
    }

/**
* @brief  s2_1_AbreFifoServidor Ler a descrição da tarefa S2.1 no enunciado
* @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
* @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
*/
void s2_1_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);
    int fd = open(filenameFifoServidor, O_RDONLY);
    if (fd == -1) { //verificar se ocorreu algu erro
        so_error("S2.1", "Erro ao abrir o FIFO do servidor para leitura");
        s4_EncerraServidor(FILE_REQUESTS);//encerrar servidor se falhar
        return;
    }
    //converter o descritor do arquivo para FILE*    
    *pfFifoServidor = fdopen(fd, "r");
    if (*pfFifoServidor == NULL) { //verificar se houve algum erro 
        close(fd);
        so_error("S2.1", "Erro ao converter descritor para FILE");
        s4_EncerraServidor(FILE_REQUESTS);// encerra servidor se falhou
        return;
    }
        
    so_success("S2.1", "FIFO do servidor aberto para leitura com sucesso");
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
* @brief  s2_2_LePedidosFifoServidor    Ler a descrição da tarefa S2.2 no enunciado
* @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
*/
void s2_2_LePedidosFifoServidor(FILE *fFifoServidor) {
    so_debug("<");
    int terminaCiclo2 = FALSE;
    while (TRUE) {
        terminaCiclo2 = s2_2_1_LePedido(fFifoServidor, &clientRequest);
        if (terminaCiclo2)
            break;
        s2_2_2_ProcuraLugarDisponivelBD(clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);
        s2_2_3_CriaServidorDedicado(lugaresEstacionamento, indexClienteBD);
    }
            
    so_debug(">");
}

/**
* @brief  s2_2_1_LePedido Ler a descrição da tarefa S2.2.1 no enunciado
* @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
* @param  pclientRequest (O) pedido recebido, enviado por um Cliente
* @return TRUE se não conseguiu ler um pedido porque o FIFO não tem mais pedidos.
*/
int s2_2_1_LePedido(FILE *fFifoServidor, Estacionamento *pclientRequest) {
    size_t bytesLidos;//armazenar quantidade de bytes lidos
    so_debug("< [@param fFifoServidor:%p]", fFifoServidor);
        
    bytesLidos = fread(pclientRequest, sizeof(Estacionamento), 1, fFifoServidor);//tenta ler um pedido do registo
        
    if (bytesLidos != 1) {//se não leu apenas um registo
        if (feof(fFifoServidor)) {//se é o fim do arquibo
            so_success("S2.2.1", "Não há mais registos no FIFO");
            clearerr(fFifoServidor);//limpa o indicador do fim do arquivo
            return TRUE;
        } else if (ferror(fFifoServidor)) { // se der erro
            so_error("S2.2.1", "Erro ao ler pedido do FIFO: %s", strerror(errno));
            clearerr(fFifoServidor); //limpa o indicador do fim do arquivo
            s4_EncerraServidor(FILE_REQUESTS);//encerra server
            return TRUE; 
        }
        return TRUE;
    }

            
    so_success("S2.2.1", "Li Pedido do FIFO");
            

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d.%d]]", pclientRequest->viatura.matricula, pclientRequest->viatura.pais, pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
    return FALSE;
}

/**
* @brief  s2_2_2_ProcuraLugarDisponivelBD Ler a descrição da tarefa S2.2.2 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
* @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
* @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
* @param  pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
*/
        
void s2_2_2_ProcuraLugarDisponivelBD(Estacionamento clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    int i;//para o for
            
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);

    *pindexClienteBD = -1;//iniciar como -1 para (não encontrou um lugar disponivel)
    for (i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == DISPONIVEL) {//se o lugar estiver disponivel
            *pindexClienteBD = i;//guarda o indice do lugar

            //preenche os dados do cliente para o lugar indicado
            lugaresEstacionamento[i].pidCliente = clientRequest.pidCliente;
            strcpy(lugaresEstacionamento[i].viatura.matricula, clientRequest.viatura.matricula);
            strcpy(lugaresEstacionamento[i].viatura.pais, clientRequest.viatura.pais);
            lugaresEstacionamento[i].viatura.categoria = clientRequest.viatura.categoria;
            strcpy(lugaresEstacionamento[i].viatura.nomeCondutor, clientRequest.viatura.nomeCondutor);       
                    
            lugaresEstacionamento[i].pidServidorDedicado = clientRequest.pidServidorDedicado;
                    
            so_success("S2.2.2", "Reservei Lugar: %d", i);
            break;//break quando preencher o lugar, se não ele pode preencher todos,
        }
    }

    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);

}

/**
* @brief  s2_2_3_CriaServidorDedicado    Ler a descrição da tarefa S2.2.3 no enunciado
* @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
* @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
*/
void s2_2_3_CriaServidorDedicado(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    pid_t pid; //PID filho(SD)
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    if (indexClienteBD < 0) {//ver se o indice é válido
        so_error("S2.2.3", "Índice inválido para criar servidor dedicado");
        return;
    }
        
    pid = fork();//cria processo filho
        
    if (pid == -1) {//verificar se houve erros
        so_error("S2.2.3", "Erro ao criar processo Servidor Dedicado");
        s4_EncerraServidor(FILE_REQUESTS);
    } else if (pid == 0) { //codigo do processo filho
        so_success("S2.2.3", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado(); //exucuta a função do servidor dedicado
    } else {//codigo pai(server principal)
        lugaresEstacionamento[indexClienteBD].pidServidorDedicado = pid;//guarda pid do servidor dedicado
        so_success("S2.2.3", "Servidor: Iniciei SD %d", pid);
    }

    so_debug(">");

}

/**
* @brief  s3_TrataCtrlC    Ler a descrição da tarefa S3 no enunciado
* @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
*/
void s3_TrataCtrlC(int sinalRecebido) {
    int i;//variavel do for
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown");//log inicio do desligamento
    for (i = 0; i < dimensaoMaximaParque; i++) {//percorre o array e notifica todos os lugares para encerrarem os SD
        //verifica se o lugar está ocupado e tem SD ativo
        if (lugaresEstacionamento[i].pidCliente != DISPONIVEL && lugaresEstacionamento[i].pidServidorDedicado > 0) {          
            if (kill(lugaresEstacionamento[i].pidServidorDedicado, SIGUSR2) == -1) {//envia sinal SIGUSR2
                so_error("S3", "Erro ao enviar SIGUSR2 ao SD PID=%d: %s", lugaresEstacionamento[i].pidServidorDedicado, strerror(errno));
                break;//break se der erro
            }
        }    
    }
        
            
    s4_EncerraServidor(FILE_REQUESTS);//encerra server principal

    so_debug(">");

}

/**
* @brief  s4_EncerraServidor    Ler a descrição da tarefa S4 no enunciado
* @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
*/
void s4_EncerraServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);
        
    //remove o file FIFO
    if (unlink(filenameFifoServidor) == -1) {
        so_error("S4", "Erro ao remover o FIFO do servidor %s: %s", filenameFifoServidor, strerror(errno));//erro
    } else {
        so_success("S4", "End Shutdown"); //log de sucesso
    }

    //liberta todos os lugares
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente != DISPONIVEL) {
            lugaresEstacionamento[i].pidCliente = DISPONIVEL;
            lugaresEstacionamento[i].pidServidorDedicado = 0;
        }
    }

    exit(0);//termina com sucesso


    so_debug(">");
}

/**
* @brief  s5_TrataTerminouServidorDedicado    Ler a descrição da tarefa S5 no enunciado
* @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
*/
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    pid_t pidFilho;//pid filho terminado
    int status; //estado da saida do filho
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    //espera por qualquer processo filho que tenha terminado
    pidFilho = wait(&status);
    if (pidFilho > 0) { //se identificou um filho
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pidFilho);//sucesso
    }

    //S6
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidServidorDedicado == pidFilho) {
            lugaresEstacionamento[i].pidCliente = DISPONIVEL;
        }
    }
            
    so_debug(">");
}

    
/**
* @brief  sd7_IniciaServidorDedicado Ler a descrição da tarefa SD7 no enunciado
*/
void sd7_MainServidorDedicado() {
    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_ValidaLugarDisponivelBD(indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSigusr1AoCliente(clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout();
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");


    so_debug(">");
        
}

/**
* @brief  sd7_1_ArmaSinaisServidorDedicado    Ler a descrição da tarefa SD7.1 no enunciado
*/
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    // Ignora o sinal SIGINT (Ctrl+C), para não ser encerrado pelo mesmo Ctrl+C do servidor principal
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Erro ao configurar o sinal SIGINT");
        exit(1);
    }
    // Configura o tratamento do sinal SIGUSR2 (para encerramento pelo servidor principal)
    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar o sinal SIGUSR2");
        exit(1);
    }
    // Configura o tratamento do sinal SIGUSR1 (para notificação de saída pelo cliente)
    if (signal(SIGUSR1, sd13_TrataSigusr1) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar o sinal SIGUSR1");
        exit(1);
    }

    so_success("SD7.1", "Sinais do Servidor Dedicado armados com sucesso");

    so_debug(">");
        
}

/**
* @brief  sd7_2_ValidaPidCliente    Ler a descrição da tarefa SD7.2 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd7_2_ValidaPidCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (clientRequest.pidCliente <= 0) { // verifica se o PID é válido
        so_error("SD7.2", "PID do cliente inválido: %d", clientRequest.pidCliente);
        exit(1);
    }

    so_success("SD7.2", "PID do cliente %d é válido", clientRequest.pidCliente);

    so_debug(">");
        
}

/**
* @brief  sd7_3_ValidaLugarDisponivelBD    Ler a descrição da tarefa SD7.3 no enunciado
* @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
*/
void sd7_3_ValidaLugarDisponivelBD(int indexClienteBD) {
    so_debug("< [@param indexClienteBD:%d]", indexClienteBD);

    if (indexClienteBD == -1) { // verifica se não há nenhum lugar disponível no estacionamento
        so_error("SD7.3", "Não há lugar disponível na BD"); // regista erro: não há lugar disponível

        if (kill(clientRequest.pidCliente, SIGHUP) == -1) { // tenta enviar o sinal SIGHUP ao cliente para notificar que não há vaga
            so_error("SD7.3", "Erro ao enviar sinal SIGHUP ao cliente %d", clientRequest.pidCliente);

        } else {// regista sucesso no envio do sinal
            so_success("SD7.3", "Enviado sinal SIGHUP ao cliente %d", clientRequest.pidCliente);
        }
        exit(1);
    }
    so_success("SD7.3", "Lugar disponível na BD no índice %d", indexClienteBD);

    so_debug(">");
        
}


/**
* @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd8_1_ValidaMatricula(Estacionamento clientRequest) {
    int i;
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (strlen(clientRequest.viatura.matricula) == 0) {// verifica se a matrícula está vazia
        so_error("SD8.1", "Matrícula vazia"); // regista erro: matrícula vazia
        sd11_EncerraServidorDedicado(); // encerra o servidor dedicado em caso de falha
    }
        
    //verificar cada caractere da matrícula
    for (i = 0; i < strlen(clientRequest.viatura.matricula); i++){ 
        char c = clientRequest.viatura.matricula[i];// caractere atual
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))){//verifica se o caractere não é letra maiúscula ou número
            so_error("SD8.1", "Matrícula contém caracteres inválidos: %c", c);
            sd11_EncerraServidorDedicado(); //encerra o servidor dedicado em caso de falha
        }
    }

    so_success("SD8.1", "Matrícula válida: %s", clientRequest.viatura.matricula);

    so_debug(">");
}

/**
* @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd8_2_ValidaPais(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (strlen(clientRequest.viatura.pais) != 2) { // verifica se o código tem exatamente duas letras
        so_error("SD8.2", "País deve ter exatamente 2 letras: %s", clientRequest.viatura.pais);
        sd11_EncerraServidorDedicado(); // Encerra o servidor dedicado em caso de falha
    }

    //verifica se ambos os caracteres são letras maiúsculas
    if (!((clientRequest.viatura.pais[0] >= 'A' && clientRequest.viatura.pais[0] <= 'Z') && (clientRequest.viatura.pais[1] >= 'A' && clientRequest.viatura.pais[1] <= 'Z'))) {
        so_error("SD8.2", "País deve conter apenas letras maiúsculas: %s", clientRequest.viatura.pais);
        sd11_EncerraServidorDedicado(); // Encerra o servidor dedicado em caso de falha
    }

    so_success("SD8.2", "País válido: %s", clientRequest.viatura.pais);

    so_debug(">");
}

/**
* @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd8_3_ValidaCategoria(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

     // verifica se a categoria é valida: P, L ou M
    if (clientRequest.viatura.categoria != 'P' && clientRequest.viatura.categoria != 'L' && clientRequest.viatura.categoria != 'M') {
        so_error("SD8.3", "Categoria inválida: %c (deve ser P, L ou M)", clientRequest.viatura.categoria);
        sd11_EncerraServidorDedicado(); // encerra o servidor dedicado em caso de falha
    }

    so_success("SD8.3", "Categoria válida: %c", clientRequest.viatura.categoria);

    so_debug(">");

}

/**
* @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd8_4_ValidaNomeCondutor(Estacionamento clientRequest) {
    FILE *fp;
    char linha[256];
    int encontrado = 0; // Flag para indicar se o nome foi encontrado
    
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", 
             clientRequest.viatura.matricula, 
             clientRequest.viatura.pais, 
             clientRequest.viatura.categoria, 
             clientRequest.viatura.nomeCondutor, 
             clientRequest.pidCliente, 
             clientRequest.pidServidorDedicado);
    
    if (strlen(clientRequest.viatura.nomeCondutor) == 0) {// verifica se o nome do condutor está vazio
        so_error("SD8.4", "Nome do condutor vazio");
        sd11_EncerraServidorDedicado(); 
    }
    
    fp = fopen("/etc/passwd", "r"); //abre o arquivo do sistema para verificar usuários
    
    if (fp == NULL) {//verifica se o arquivo foi aberto com sucesso
        so_error("SD8.4", "Erro ao abrir o ficheiro de usuários.");
        sd11_EncerraServidorDedicado(); 
    }
    
    //ler cada linha do arquivo de usuarios
    while(so_fgets(linha, sizeof(linha), fp) != NULL) {
        char *token = strtok(linha, ":"); //divide a linha pelo caractere ':'
        if (token == NULL) continue;//se não conseguir dividir, continua para a proxima linha
        
    // avança até o quinto campo (índice 4), que contém o nome completo
    for(int i = 0; i < 4; i++) {
        token = strtok(NULL, ":");//próximo campo
        if(token == NULL) break;//se acabarem os campos, break
    }
        
    if(token != NULL) {//se encontrou o campo do nome
        // Remove o newline se existir
        char *nl_ptr = strchr(token, '\n');
        if (nl_ptr) *nl_ptr = '\0'; //substitui a vírgula por fim de string
            
        // Remove as vírgulas no final do nome
        char *comma = strchr(token, ',');
        if (comma) *comma = '\0';
            
        // Compara o nome do condutor com o nome no arquivo
        if(strcmp(token, clientRequest.viatura.nomeCondutor) == 0) {
            so_success("SD8.4", "Nome do condutor válido %s", clientRequest.viatura.nomeCondutor);
            encontrado = 1; // Marca como encontrado
            break;  // Sai ao encontrar o nome
            }
        }
    }
    
    fclose(fp);
    
    // Só exibe o erro se o nome não foi encontrado
    if (!encontrado) {
        so_error("SD8.4", "Nome do condutor não encontrado %s", clientRequest.viatura.nomeCondutor);
        sd11_EncerraServidorDedicado(); 
    }
}


/**
* @brief  sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
*/
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");
    
    int tempEsp = so_random_between_values(1, MAX_ESPERA);
    
    so_success("SD9.1", "%d", tempEsp);
    sleep(tempEsp);//pausa a execução pelo tempo definido

    so_debug(">");
}

/**
* @brief  sd9_2_EnviaSigusr1AoCliente Ler a descrição da tarefa SD9.2 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd9_2_EnviaSigusr1AoCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (clientRequest.pidCliente <= 0) { //Verifica se o PID do cliente é valido
        so_error("SD9.2", "PID do cliente inválido: %d", clientRequest.pidCliente);
        sd11_EncerraServidorDedicado();
        return;
    }
        
    // Enviar o sinal SIGUSR1 ao cliente para confirmar lugar
    if (kill(clientRequest.pidCliente, SIGUSR1) == -1) {//tenta enviar o sinal SIGUSR1 para notificar que o lugar foi reservado
        so_error("SD9.2", "Erro ao enviar sinal SIGUSR1 ao cliente %d: %s", clientRequest.pidCliente, strerror(errno));
        sd11_EncerraServidorDedicado();
    } else {
        so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD); 
    }

    so_debug(">");
}

/**
* @brief  sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
* @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
* @param  pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
* @param  plogItem (O) registo de Log para esta viatura
*/
void sd9_3_EscreveLogEntradaViatura(char *logFilename, Estacionamento clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]", logFilename, clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    FILE *fp = fopen(logFilename, "a+");
    if (fp == NULL) {//verifica se o arquivo foi aberto com sucesso
        so_error("SD9.3", "Erro ao abrir o ficheiro %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    fseek(fp, 0, SEEK_END); //move o cursor para o final do arquivo
    *pposicaoLogfile = ftell(fp); //obtém a posição atual (será usada para atualizar o log na saída)

    // Copia os dados da viatura para o registo de log
    strcpy(plogItem->viatura.matricula, clientRequest.viatura.matricula);
    strcpy(plogItem->viatura.pais, clientRequest.viatura.pais);
    plogItem->viatura.categoria = clientRequest.viatura.categoria;
    strcpy(plogItem->viatura.nomeCondutor, clientRequest.viatura.nomeCondutor);

    time_t now = time(NULL);//tempo atual
    struct tm *tm_info = localtime(&now); //converte para o formato local
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", tm_info); //formata a data/hora 

    strcpy(plogItem->dataSaida, "");//inicia a data de saída como vazia

    // Escreve o registo de log no arquivo
    if (fwrite(plogItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD9.3", "Erro ao escrever no ficheiro %s", logFilename);
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }

    fclose(fp);//fecha o arquivo


    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s", *pposicaoLogfile, clientRequest.viatura.matricula, plogItem->dataEntrada);

    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}

/**
* @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
*/
void sd10_1_AguardaCheckout() {
    so_debug("<");
        
    pause(); //pausa a execução até receber um sinal (espera que o cliente inicie o checkout)
        
    so_success("SD10.1", "SD: A viatura %s deseja sair do parque", clientRequest.viatura.matricula);

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

    FILE *fp = fopen(logFilename, "r+"); //abre o arquivo de log em modo leitura/escrita binário
    if (fp == NULL) {  //verifica se o arquivo foi aberto com sucesso
        so_error("SD10.2", "Erro ao abrir o ficheiro %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) {//posiciona o cursor na posição onde o registo foi gravado na entrada
        so_error("SD10.2", "Erro ao posicionar o ficheiro na posição %ld", posicaoLogfile);
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }
        
    time_t now = time(NULL); //tempo atual
    struct tm *tm_info = localtime(&now); //formato local
    strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", tm_info); //Formata a data/hora de saída
        
    if (fwrite(&logItem, sizeof(LogItem), 1, fp) != 1) {  //atualiza o registo com a data de saída
        so_error("SD10.2", "Erro ao escrever no ficheiro %s", logFilename);
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }
    fclose(fp);
        
    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s", posicaoLogfile, logItem.viatura.matricula, logItem.dataSaida);
    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
* @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
*         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
*/
void sd11_EncerraServidorDedicado() {
    so_debug("<");
    sd11_1_LibertaLugarViatura(lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaSighupAoClienteETermina(clientRequest);

    so_debug(">");
}

/**
* @brief  sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
* @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
* @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
*/
void sd11_1_LibertaLugarViatura(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    if (indexClienteBD < 0) {  //verifica se o índice é válido
        so_error("SD11.1", "Índice inválido: %d", indexClienteBD);
    }
    
    strcpy(lugaresEstacionamento[indexClienteBD].viatura.matricula, "");
    strcpy(lugaresEstacionamento[indexClienteBD].viatura.pais, "");
    lugaresEstacionamento[indexClienteBD].viatura.categoria = '\0';
    strcpy(lugaresEstacionamento[indexClienteBD].viatura.nomeCondutor, "");
    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = 0;
        
    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);

    so_debug(">");
}

/**
* @brief  sd11_2_EnviaSighupAoClienteETerminaSD Ler a descrição da tarefa SD11.2 no enunciado
* @param  clientRequest (I) pedido recebido, enviado por um Cliente
*/
void sd11_2_EnviaSighupAoClienteETermina(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (kill(clientRequest.pidCliente, SIGHUP) == -1) {// Tenta enviar o sinal SIGHUP ao cliente para notificar que o servidor dedicado está encerrando
        so_error("SD11.2", "Erro ao enviar sinal SIGHUP ao cliente %d", clientRequest.pidCliente);
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

    so_success("SD12", "SD: Recebi pedido do Servidor para terminar"); // regista que recebeu sinal do servidor principal para terminar
        
    sd11_EncerraServidorDedicado();//inicia o encerramento do seridor

    so_debug(">");
}


/**
* @brief  sd13_TrataSigusr1    Ler a descrição da tarefa SD13 no enunciado
* @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
*/
void sd13_TrataSigusr1(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD13", "SD: Recebi pedido do Cliente para terminar o estacionamento");  //regista que recebeu sinal do cliente para terminar o estacionamento

    so_debug(">");
}  