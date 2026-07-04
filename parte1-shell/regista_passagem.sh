#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº: 129850      Nome: Gonçalo Sobral
## Nome do Módulo: S1. Script: regista_passagem.sh
## Descrição/Explicação do Módulo: Valida os argumentos de entrada (matrícula, código do país, categoria e condutor); Verifica se a matrícula está em conformidade com as regras do país indicado
## Confirma se o veículo pode entrar ou sair do estacionamento (para evitar entradas/saídas duplicadas); Regista as entradas e saídas no ficheiro estacionamentos.txt
## Ordena os registos por hora de entrada em um ficheiro separado (estacionamentos-ordenados-hora.txt)  
#####################################################################################

## Este script é invocado quando uma viatura entra/sai do estacionamento Park-IUL. Este script recebe todos os dados por argumento, na chamada da linha de comandos, incluindo os <Matrícula:string>, <Código País:string>, <Categoria:char> e <Nome do Condutor:string>.

## S1.1. Valida os argumentos passados e os seus formatos:
## • Valida se os argumentos passados são em número suficiente (para os dois casos exemplificados), assim como se a formatação de cada argumento corresponde à especificação indicada. O argumento <Categoria> pode ter valores: L (correspondente a Ligeiros), P (correspondente a Pesados) ou M (correspondente a Motociclos);
#validar nº de argumentos

if [[ $# -lt 1 || $# -gt 4 ]]; then #verificar se os argumentos fornecidos são 1 ou 4
    so_error S1.1 "Nº de argumentos inválido"
    exit 1  
fi

if [[ ! -f paises.txt ]]; then #procurar ficheiro dos paises
    so_error S1.1 "Ficheiro paises.txt não encontrado"
    exit 1
fi

#variaveis
MATRICULA=$1
PAIS=$2
CATEGORIA=$3
CONDUTOR=$4

#se for saida reg
if [[ $# -eq 1 ]]; then
    ARGUMENTO=$(echo "$1" | tr -d '[:space:]') 
    if [[ ! "$ARGUMENTO" =~ .*/.* ]]; then #se o argumento não for separado por uma /
        so_error S1.1 "Formato inválido: deve usar PAIS/MATRICULA"
        exit 1
    fi
    PAIS=$(echo "$ARGUMENTO" | cut -d'/' -f1)
    MATRICULA=$(echo "$ARGUMENTO" | cut -d'/' -f2)

    if [[ -z "$PAIS" ]]; then #ver se $pais esta vazio
        so_error S1.1 "Código de país não especificado"
        exit 1
    fi
    
    if [[ -z "$MATRICULA" ]]; then # "" $matricula"
        so_error S1.1 "Matrícula não especificada"
        exit 1
    fi
    REG_SAIDA=true

else
    REG_SAIDA=false
fi

## • A partir da indicação do argumento <Código País>, valida se o argumento <Matrícula> passada cumpre a especificação da correspondente <Regra Validação Matrícula>;
#validar se o pais corresponde com a matricula indicada
PAIS=$(echo "$PAIS" | tr -d '[:space:]')
PAISREG=$(grep "^$PAIS###" paises.txt | awk -F '###' '{print $3}')

if [[ -z "$PAISREG" ]]; then #se o pais não existe dá erro
    so_error S1.1 "Pais não está registado"
    exit 1
fi

#remover ifens para armazenar
MATRICULA_A=$MATRICULA
MATRICULA=$(echo "$MATRICULA" | tr -d ' -')


if [[ ! "$MATRICULA" =~ $PAISREG ]]; then #ver se a matricula esta correta, de acordo com o seu pais
    so_error S1.1 "Matricula inválida para $PAIS"
    exit 1
fi


#VERIFICAR SE A CATEGORIA É L,M OU P
if [[ $# -eq 4 && ! "$CATEGORIA" =~ ^[LMP]$ ]]; then
    so_error S1.1 "Categoria não existe"
    exit 1
fi

USER_FILE="_etc_passwd"


if [[ ! -f "$USER_FILE" ]]; then 
    so_error S1.1 "Ficheiro de utilizadores não encontrado em $USER_FILE"
    exit 1
fi

if [[ $# -eq 4 ]]; then 
    NOME_C=$(grep -E "^[^:]*:[^:]*:[^:]*:[^:]*:${CONDUTOR}:" "$USER_FILE") #tirar nome completo
    
    if [[ -z "$NOME_C" ]]; then 
        FIRST_N=$(echo "$CONDUTOR" | awk '{print $1}') #1º nome, embaixo o ultimo nome
        LAST_N=$(echo "$CONDUTOR" | awk '{print $NF}')
        
        
        if [[ "$(echo "$CONDUTOR" | wc -w)" -ne 2 ]] || [[ -z "$(grep -E "^[^:]*:[^:]*:[^:]*:[^:]*:[^:]*${FIRST_N}[^:]*${LAST_N}[^:]*:" "$USER_FILE")" ]]; then
            so_error S1.1 "Nome de condutor inválido"
            exit 1
        fi
    fi
fi





## • Em caso de qualquer erro das condições anteriores, dá so_error S1.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.1.
so_success S1.1 "Argumentos validados"

## S1.2. Valida os dados passados por argumento para o script com o estado da base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:
## • Valida se, no caso de a invocação do script corresponder a uma entrada no parque de estacionamento, se ainda não existe nenhum registo desta viatura na base de dados;
## • Valida se, no caso de a invocação do script corresponder a uma saída do parque de estacionamento, se existe um registo desta viatura na base de dados;
if [[ ! -f estacionamentos.txt ]]; then
    touch estacionamentos.txt
fi

REG_EX=$(grep "^$MATRICULA:" estacionamentos.txt)  #ver se existe a matricula no ficheiro

if $REG_SAIDA; then #reg saida
    if [[ -z "$REG_EX" ]]; then 
        so_error S1.2 "Matricula não registou nenhuma entrada"
        exit 1
    fi
    ULTIMO_REG=$(echo "$REG_EX" | tail -1) 
    if [[ "$ULTIMO_REG" == *:*:*:*:*:* ]]; then #se tiver 6 arg já saiu
        so_error S1.2 "O veículo não está no parque"
        exit 1
    fi
else  #reg de entrada
    if [[ ! -z "$REG_EX" ]]; then #
        ULTIMO_REG=$(echo "$REG_EX" | tail -1)
        if [[ "$ULTIMO_REG" != *:*:*:*:*:* ]]; then #se não tiver 6 arg ainda não saiu
            so_error S1.2 "O veículo já está no parque"
            exit 1
        fi
    fi
fi

so_success S1.2 "Validação concluída"



## • Em caso de qualquer erro das condições anteriores, dá so_error S1.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.2.


## S1.3. Atualiza a base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:
## • Remova do argumento <Matrícula> passado todos os separadores (todos os caracteres que não sejam letras ou números) eventualmente especificados;
#já feito anteriormente

## • Especifique como data registada (de entrada ou de saída, conforme o caso) a data e hora do sistema Tigre;
DATA_AT=$(date +"%Y-%m-%dT%Hh%M") 

## • No caso de um registo de entrada, crie um novo registo desta viatura na base de dados;
if $REG_SAIDA; then
    if ! sed -i "s/^\($MATRICULA:[^:]*:[^:]*:[^:]*:[^:]*\)$/\1:$DATA_AT/" estacionamentos.txt; then  
        so_error S1.3 "Erro ao registar a saída"
        exit 1
    fi
else
    # Para entrada, criamos um novo registro
    if ! echo "$MATRICULA:$PAIS:$CATEGORIA:$CONDUTOR:$DATA_AT" >> estacionamentos.txt; then
        so_error S1.3 "Erro ao registar o estacionamento"
        exit 1
    fi
fi

so_success S1.3 "Registo feito com sucesso"
## • No caso de um registo de saída, atualize o registo desta viatura na base de dados, registando a data de saída;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.3 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.3.


## S1.4. Lista todos os estacionamentos registados, mas ordenados por saldo:
## • O script deve criar um ficheiro chamado estacionamentos-ordenados-hora.txt igual ao que está no ficheiro estacionamentos.txt, com a mesma formatação, mas com os registos ordenados por ordem crescente da hora (e não da data) de entrada das viaturas.


if [[ -f estacionamentos.txt ]]; then
    awk -F: '{
    split($5, time, "T"); #separar dia e hora
       
        hour = substr(time[2], 1, 2); 
        minute = substr(time[2], 4, 2);

        p = hour minute; 

        print p " " $0;
    }' estacionamentos.txt | 

    sort -n | 

    cut -d " " -f 2- > estacionamentos-ordenados-hora.txt
    
    
    if [[ $? -ne 0 ]]; then
        so_error S1.4 "Erro ao ordenar estacionamentos por hora"
        exit 1
    fi
fi

so_success S1.4 "Ordem por hora alterada"
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.4 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.4.
