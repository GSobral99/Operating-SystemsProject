#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129850       Nome:Gonçalo Sobral
## Nome do Módulo: S2. Script: manutencao.sh
## Descrição/Explicação do Módulo: Valida a integridade dos dados no ficheiro estacionamentos.txt; 
## Processa os registos completos (com entrada e saída) transferindo-os do ficheiro ativo para arquivos mensais;
## Mantém no ficheiro estacionamentos.txt apenas os registos de veículos que ainda não saíram do parque
#####################################################################################

## Este script não recebe nenhum argumento, e permite realizar a manutenção dos registos de estacionamento. 

## S2.1. Validações do script:
## O script valida se, no ficheiro estacionamentos.txt:
## • Todos os registos referem códigos de países existentes no ficheiro paises.txt;
## • Todas as matrículas registadas correspondem à especificação de formato dos países correspondentes;
## • Todos os registos têm uma data de saída superior à data de entrada;
if [[ ! -f "paises.txt" ]]; then
    so_error S2.1 "O ficheiro paises.txt não existe"
    exit
fi

if [[ ! -f "estacionamentos.txt" ]]; then
    so_success S2.1
    so_success S2.2
    exit 0
fi

if [[ ! -s "estacionamentos.txt" ]]; then
    so_success S2.1
    so_success S2.2
    exit 0
fi

if [[ ! -w "." ]]; then
    so_success S2.1
    so_error S2.2 "Diretoria local sem permissões para escrita"
    exit 1
fi


> estacionamentos_t.txt #criar ficheiro temporário para o original não ficar com linhas em branco

while read -r linha; do
    MATRICULA=$(echo "$linha" | cut -d':' -f1)
    PAIS=$(echo "$linha" | cut -d':' -f2)
    CATEGORIA=$(echo "$linha" | cut -d':' -f3)
    CONDUTOR=$(echo "$linha" | cut -d':' -f4)
    DATA_E=$(echo "$linha" | cut -d':' -f5) #data de entrada
    DATA_S=$(echo "$linha" | cut -d':' -f6) #data de saida
    PAIS_EX=$(grep -F "$PAIS###" paises.txt) #ver se o pais existe


    if [[ -z "$PAIS_EX" ]]; then
        so_error S2.1 "O país $PAIS na matricula $MATRICULA não existe no ficheiro de registos"
        rm estacionamentos_t.txt
        exit 1
    fi

    PAIS_REG=$(grep "^$PAIS###" paises.txt | awk -F '###' '{print $3}') #normas da matricula de acordo com o pais

    if [[ ! "$MATRICULA" =~ $PAIS_REG ]]; then
        so_error S2.1 "A matricula $MATRICULA é inválida para para o pais: $PAIS"
        exit 1
    fi

    if [[ -n "$DATA_S" ]]; then
        #tirar Hora e Minutos e retirar letras
        DATA_E_F=$(echo "$DATA_E" | sed 's/T/ /; s/h/:/') 
        DATA_S_F=$(echo "$DATA_S" | sed 's/T/ /; s/h/:/')
        #trasformar a data em segundos 
        DATA_E_B=$(date -d "${DATA_E_F//./-}" +%s) 
        DATA_S_B=$(date -d "${DATA_S_F//./-}" +%s)
        if [[ "$DATA_S_B" -le "$DATA_E_B" ]]; then
            so_error S2.1 "Data de saida inválida para $MATRICULA "
            rm estacionamentos_t.txt
            exit 1
        fi
    fi


 # • Em caso de qualquer erro das condições anteriores, dá so_error S2.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.1.

    
 # S2.2. Processamento:
 # • O script move, do ficheiro estacionamentos.txt, todos os registos que estejam completos (com registo de entrada e registo de saída), mantendo o formato do ficheiro original, para ficheiros separados com o nome arquivo-<Ano>-<Mês>.park, com todos os registos agrupados pelo ano e mês indicados pelo nome do ficheiro. Ou seja, os registos são removidos do ficheiro estacionamentos.txt e acrescentados ao correspondente ficheiro arquivo-<Ano>-<Mês>.park, sendo que o ano e mês em questão são os do campo <DataSaída>. 
    
    
    if [[ -n "$DATA_S" ]]; then #se existir data de saida
        TEMPO_T=$(( (DATA_S_B - DATA_E_B) / 60)) #calcular tempo em minutos


        ANO_MES=$(echo "$DATA_S" | cut -d'-' -f1,2) 
        N_ARQUIVO="arquivo-${ANO_MES}.park"

        echo "$MATRICULA:$PAIS:$CATEGORIA:$CONDUTOR:$DATA_E:$DATA_S:$TEMPO_T" >> "$N_ARQUIVO"  #escrever no arquivo

    else #se não tiver data de saida, ainda não saiu do parque
        echo "$linha" >> estacionamentos_t.txt #escrever no temporario
    fi
done < estacionamentos.txt
so_success S2.1 

mv estacionamentos_t.txt estacionamentos.txt #transformar o temporario no permanente

so_success S2.2


## • Quando acrescentar o registo ao ficheiro arquivo-<Ano>-<Mês>.park, este script acrescenta um campo <TempoParkMinutos> no final do registo, que corresponde ao tempo, em minutos, que durou esse registo de estacionamento (correspondente à diferença em minutos entre os dois campos anteriores).
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.2.
## • O registo em cada ficheiro arquivo-<Ano>-<Mês>.park, tem então o formato:
## <Matrícula:string>:<Código País:string>:<Categoria:char>:<Nome do Condutor:string>: <DataEntrada:AAAA-MM-DDTHHhmm>:<DataSaída:AAAA-MM-DDTHHhmm>:<TempoParkMinutos:int>
## • Exemplo de um ficheiro arquivo-<Ano>-<Mês>.park, para janeiro de 2025:

