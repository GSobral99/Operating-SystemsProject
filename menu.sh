#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129850       Nome: Gonçalo Sobral
## Nome do Módulo: S4. Script: menu.sh
## Descrição/Explicação do Módulo: Apresenta um menu principal; Recolhe dados do utilizador conforme a opção selecionada;
## Encaminha os dados recolhidos para os scripts especializados; Mantém uma execução cíclica até que o utilizador pretenda sair (opção 0)
##
#####################################################################################

## Este script invoca os scripts restantes, não recebendo argumentos.
## Atenção: Não é suposto que volte a fazer nenhuma das funcionalidades dos scripts anteriores. O propósito aqui é simplesmente termos uma forma centralizada de invocar os restantes scripts.
## S4.1. Apresentação:
## S4.1.1. O script apresenta (pode usar echo, cat ou outro, sem “limpar” o ecrã) um menu com as opções abaixo indicadas.
display() {
    echo "Menu:"
    echo "1: Regista passagem - Entrada Estacionamento"
    echo "2: Regista passagem - Saída Estacionamento"
    echo "3: Manutenção"
    echo "4: Estatísticas:"
    echo "0: Sair"
    echo -n "Opção: "  
}

## S4.2. Validações:
## S4.2.1. Aceita como input do utilizador um número. Valida que a opção introduzida corresponde a uma opção válida. Se não for, dá so_error <opção> (com a opção errada escolhida), e volta ao passo S4.1 (ou seja, mostra novamente o menu). Caso contrário, dá so_success <opção>.




## S4.2.2. Analisa a opção escolhida, e mediante cada uma delas, deverá invocar o sub-script correspondente descrito nos pontos S1 a S3 acima. No caso das opções 1 e 4, este script deverá pedir interactivamente ao utilizador as informações necessárias para execução do sub-script correspondente, injetando as mesmas como argumentos desse sub-script:
## S4.2.2.1. Assim sendo, no caso da opção 1, o script deverá pedir ao utilizador sucessivamente e interactivamente os dados a inserir:
registo_entrada() {
    echo "Regista passagem de Entrada estacionamento:"
    echo -n "Indique a matrícula da viatura: "
    read matricula
    echo -n "Indique o código do país de origem da viatura: "
    read pais
    echo -n "Indique a categoria da viatura [L(igeiro)|P(esado)|M(otociclo)]: "
    read categoria
    echo -n "Indique o nome do condutor da viatura: "
    read condutor
    
    ./regista_passagem.sh "$matricula" "$pais" "$categoria" "$condutor"
    so_success "S4.3" "Passagem da entrada registada"
}

## Este script não deverá fazer qualquer validação dos dados inseridos, já que essa validação é feita no script S1. Após receber os dados, este script invoca o Sub-Script: regista_passagem.sh com os argumentos recolhidos do utilizador. Após a execução do sub-script, dá so_success e volta ao passo S4.1.
## S4.2.2.2. No caso da opção 2, o script deverá pedir ao utilizador sucessivamente e interactivamente os dados a inserir:
##  Este script não deverá fazer qualquer validação dos dados inseridos, já que essa validação é feita no script S1. Após receber os dados, este script invoca o Sub-Script: regista_passagem.sh com os argumentos recolhidos do utilizador. Após a execução do sub-script, dá so_success e volta ao passo S4.1.
registo_saida() {
    echo "Regista passagem de Saída estacionamento:"
    echo -n "Indique a matrícula da viatura: "
    read matricula
    echo -n "Indique o código do país de origem da viatura: "
    read pais
    
    ./regista_passagem.sh "$pais"/"$matricula"
    so_success "S4.4" "Passagem da entrada registada"
}

## S4.2.2.3. No caso da opção 3, o script invoca o Sub-Script: manutencao.sh. Após a execução do sub-script, dá so_success e volta para o passo S4.1.
manutencao() {
    ./manutencao.sh
    so_success "S4.5" "Manutenção concluída"
}
## S4.2.2.4. No caso da opção 4, o script deverá pedir ao utilizador as opções de estatísticas a pedir, antes de invocar o Sub-Script: stats.sh. Se uma das opções escolhidas for a 8, o menu deverá invocar o Sub-Script: stats.sh sem argumentos, para que possa executar TODAS as estatísticas, caso contrário deve respeitar a ordem.
estatisticas() {
    echo "1: matriculas e condutores cujas viaturas estão ainda estacionadas no parque"
    echo "2: top3 das matrículas das viaturas que passaram mais tempo estacionado"
    echo "3: tempos de estacionamento de ligeiros e pesados agrupadas por país"
    echo "4: top3 das matrículas das viaturas que estacionaram mais tarde num dia"
    echo "5:  tempo total de estacionamento por utilizador"
    echo "6:  tempo total de estacionamento por utilizador"
    echo "7:  tempo total de estacionamento por utilizador"
    echo "8:  tempo total de estacionamento por utilizador"

    read -a opcoes

    if [ ${#opcoes[@]} -eq 0 ]; then
        echo "Menu call to script: './stats.sh' with ERROR using 0 argument(s): "
        so_error S4.6
        return
    fi

    if [[ " ${opcoes[@]} " =~ " 8 " ]]; then
        echo "Menu call to script: './stats.sh' with SUCCESS using 0 argument(s): "
        ./stats.sh
    else        
        echo "Menu call to script: './stats.sh' with SUCCESS using ${#opcoes[@]} argument(s): ${opcoes[@]}"
        ./stats.sh "${opcoes[@]}"
    fi


    so_success S4.6

}

opcao=""
while true; do
    display
    read -r opcao

    case "$opcao" in
        1)
            so_success "S4.2.1" "$opcao"
            registo_entrada
            ;;
        2)
            so_success "S4.2.1" "$opcao"
            registo_saida
            ;;
        3)
            so_success "S4.2.1" "$opcao"
            manutencao
            ;;
        4)
            so_success "S4.2.1" "$opcao"
            estatisticas
            ;;
        0)
            so_success "S4.2.1" "$opcao"
            echo "A sair ..."
            exit 0
            ;;
        *)
            so_error "S4.2.1" "$opcao"
            ;;
    esac
done


## Após a execução do Sub-Script: stats.sh, dá so_success e volta para o passo S4.1.


## Apenas a opção 0 (zero) permite sair deste Script: menu.sh. Até escolher esta opção, o menu deverá ficar em ciclo, permitindo realizar múltiplas operações iterativamente (e não recursivamente, ou seja, não deverá chamar o Script: menu.sh novamente). 

