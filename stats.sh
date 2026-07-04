#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129850       Nome:Gonçalo Sobral
## Nome do Módulo: S3. Script: stats.sh
## Descrição/Explicação do Módulo: Valida a existência e acessibilidade dos arquivos necessários (arquivo-*.park, paises.txt, estacionamentos.txt)
## Processa os dados dos estacionamentos e gera diferentes tipos de estatísticas:
    #Stats1: Lista de veículos atualmente estacionados (sem saída registada)
    #Stats2: Top 3 veículos que permaneceram mais tempo estacionados
    #Stats3: Tempo total de estacionamento por país, excluindo motociclos
    #Stats4: 3 estacionamentos mais recentes ordenados por hora de entrada
    ##Stats5: Tempo total de estacionamento por condutor, convertido em dias, horas e minutos
    #Stats6: Tempo total de estacionamento por matrícula, agrupado por país
    #Stats7: Top 3 condutores com os nomes mais compridos
## Cria uma página HTML (stats.html) formatada com as estatísticas solicitadas, que podem ser especificadas como parâmetros do script (1-7) ou todas geradas se nenhum parâmetro for fornecido
#####################################################################################

## Este script obtém informações sobre o sistema Park-IUL, afixando os resultados das estatísticas pedidas no formato standard HTML no Standard Output e no ficheiro stats.html. Cada invocação deste script apaga e cria de novo o ficheiro stats.html, e poderá resultar em uma ou várias estatísticas a serem produzidas, todas elas deverão ser guardadas no mesmo ficheiro stats.html, pela ordem que foram especificadas pelos argumentos do script.

## S3.1. Validações:
## O script valida se, na diretoria atual, existe algum ficheiro com o nome arquivo-<Ano>-<Mês>.park, gerado pelo Script: manutencao.sh. Se não existirem ou não puderem ser lidos, dá so_error S3.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S3.1.
if [[ -z $(find . -name "arquivo-*.park") ]]; then
    so_error S3.1 "Não foi encontrado nenhum arquivo arquivo-<Ano>-<Mês>.park"
    exit 1
fi

if [[ ! -r "paises.txt" ]]; then
    so_error S3.1 "Ficheiro paises.txt não existe ou não pode ser lido"
    exit 1
fi

if [[ ! -r "estacionamentos.txt" ]]; then
    so_error S3.1 "Ficheiro estacionamentos.txt não existe ou não pode ser lido"
    exit 1
fi

if [[ -z $(find . -name "arquivo-*.park") ]]; then
    so_error S3.1 "Ficheiro arquivo-*.park não existe ou não pode ser lido"
    exit 1
fi

for arquivo in arquivo-*.park; do 
    if [[ ! -r "$arquivo" ]]; then
        so_error S3.1 "Ficheiro $arquivo não pode ser lido"
        exit 1
    fi
done


if [[ $# -gt 0 ]]; then #ver os argumentos
    for arg in "$@"; do
        if ! [[ "$arg" =~ ^[1-7]$ ]]; then #verificar se os argumentos estão entre 1 e 7
            so_error S3.1 "Estatisticas inválidas: $arg"
            exit 1
        fi
    done
fi
so_success S3.1 "Ficheiro(s) encontrados"


## S3.2. Estatísticas:
## Cada uma das estatísticas seguintes diz respeito à extração de informação dos ficheiros do sistema Park-IUL. Caso não haja informação suficiente para preencher a estatística, poderá apresentar uma lista vazia.
DATA_AT=$(date +"%Y-%m-%d %H:%M:%S")
echo '<html><head><meta charset="UTF-8"><title>Park-IUL: Estatísticas de estacionamento</title></head>' > stats.html
echo "<body><h1>Lista atualizada em $DATA_AT</h1>">> stats.html


## S3.2.1.  Obter uma lista das matrículas e dos nomes de todos os condutores cujas viaturas estão ainda estacionadas no parque, ordenados alfabeticamente por nome de condutor:
stats1() {
    echo "<h2>Stats1:</h2>" >> stats.html
    echo "<ul>" >> stats.html
    awk -F':' 'NF == 5 {print $4 ":" $1}' estacionamentos.txt | sort | #retirar nome e a matricula e ordena alfabeticamente
    while IFS=':' read -r nome matricula; do #vai retirando os nomes e matriculas das linhas
        echo "<li><b>Matrícula:</b> $matricula <b>Condutor:</b> $nome</li>" >> stats.html #adiciona ao ficheiro html
    done
    echo "</ul>" >> stats.html
}
## <h2>Stats1:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## ...
## </ul>


## S3.2.2. Obter uma lista do top3 das matrículas e do tempo estacionado das viaturas que já terminaram o estacionamento e passaram mais tempo estacionadas, ordenados decrescentemente pelo tempo de estacionamento (considere apenas os estacionamentos cujos tempos já foram calculados):
stats2() {
    echo "<h2>Stats2:</h2>" >> stats.html
    echo "<ul>" >> stats.html
    awk -F':' '
    NF >= 7 {
        tempo[$1] += $7 #soma o tempo ($7) à matricula ($1)
    }
    END {
        for (matricula in tempo) {
            print tempo[matricula] ":" matricula #imprime tempo:matricula
        }
    }' arquivo-*.park | sort -nr | head -3 | while IFS=':' read -r tempo_total matricula; do #ordena por tempo decrescente
        echo "<li><b>Matrícula:</b> $matricula <b>Tempo estacionado:</b> $tempo_total</li>" >> stats.html #escreve no ficheiro
    done
    echo "</ul>" >> stats.html
}
## <h2>Stats2:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## </ul>

## S3.2.3. Obter as somas dos tempos de estacionamento das viaturas que não são motociclos, agrupadas pelo nome do país da matrícula (considere apenas os estacionamentos cujos tempos já foram calculados):

stats3() {
    echo "<h2>Stats3:</h2>" >> stats.html
    echo "<ul>" >> stats.html
    awk -F":" '$3 != "M" {print $2, $7}' arquivo-*.park | #verifica que a categoria não é M e imprime Pais e Tempo
    awk '
    {
        tempo[$1] += $2 #soma o tempo por pais
    }
    END {
        for (p in tempo) { 
            nome_pais = p #transformar a abreviatura no nome por extenso
            if (p == "PT") nome_pais = "Portugal" 
            else if (p == "UK") nome_pais = "Reino Unido"
            else if (p == "ES") nome_pais = "Espanha"
            print "<li><b>País:</b> " nome_pais " <b>Total tempo estacionado:</b> " tempo[p] "</li>" #escrever no ficheiro
        }
    }' >> stats.html

    echo "</ul>" >> stats.html

}


## <h2>Stats3:</h2>
## <ul>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>


## S3.2.4. Listar a matrícula, código de país e data de entrada dos 3 estacionamentos, já terminados ou não, que registaram uma entrada mais tarde (hora de entrada) no parque de estacionamento, ordenados crescentemente por hora de entrada:
stats4() {
    echo "<h2>Stats4:</h2>" >> stats.html
    echo "<ul>" >> stats.html
    awk -F: '
    NF >= 5 {
        split($5, data, "T") #separar Hora de Min
        hora = data[2] #escreve só a hora
        print hora ";" $5 ";" $1 ";" $2 # Imprime: hora;data;matrícula;país
    }' estacionamentos.txt arquivo-*.park | sort -t";" -k1 | tail -n 3 | sort -t";" -k1 | # ordenar por tempo e retirar os 3 ultimos e voltar a oredenar por hora
    while IFS=";" read hora entrada matricula pais; do
        echo "<li><b>Matrícula:</b> $matricula <b>País:</b> $pais <b>Data Entrada:</b> $entrada</li>" #escrever no ficheiro html
    done >> stats.html

    echo "</ul>" >> stats.html
}

## <h2>Stats4:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## </ul>


## S3.2.5. Tendo em consideração que um utilizador poderá ter várias viaturas, determine o tempo total, medido em dias, horas e minutos gasto por cada utilizador da plataforma (ou seja, agrupe os minutos em dias e horas).
stats5() {
    echo "<h2>Stats5:</h2>" >> stats.html
    echo "<ul>" >> stats.html
    cat estacionamentos.txt arquivo-*.park | awk -F: 'NF >= 7 { tempos[$4] += $7 } #somar o tempo por condutor
    END {
        for (c in tempos) {
            total = tempos[c]
            d = int(total / 1440) 
            h = int((total % 1440) / 60)
            m = total % 60
            printf "<li><b>Condutor:</b> %s <b>Tempo total:</b> %d dia(s), %d hora(s) e %d minuto(s)</li>\n", c, d, h, m # escrever no ficheiro
        }
    }' | sort >> stats.html #ordenar por ordem alfabetica
    echo "</ul>" >> stats.html
}

## <h2>Stats5:</h2>
## <ul>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## ...
## </ul>


## S3.2.6. Liste as matrículas das viaturas distintas e o tempo total de estacionamento de cada uma, agrupadas pelo nome do país com um totalizador de tempo de estacionamento por grupo, e totalizador de tempo global.
stats6() {
    echo "<h2>Stats6:</h2>" >> stats.html
    echo "<ul>" >> stats.html
     awk -F: '
    {
        matricula = $1
        pais = $2
        tempo = $7 + 0  # converte para número
        total_por_viatura[pais, matricula] += tempo #soma o tempo por matricula
        total_por_pais[pais] += tempo #soma o total por pais
        paises[pais] = 1 #regista o países unicos
    }
    END {
        while ((getline < "paises.txt") > 0) { #lê uma linha por vez
            split($0, a, "###")  #separa a linha em campos
            nome_pais[a[1]] = a[2] #mapeia omcodigo para nome completo
        }
        for (p in paises) {
            total_pais = total_por_pais[p] 
            print "<li><b>País:</b> " nome_pais[p] " <b>Total tempo estacionado:</b> " total_pais "</li>"
            print "<ul>"
            for (key in total_por_viatura) {
                split(key, k, SUBSEP) #divide a chave
                if (k[1] == p) { #se o pais da key corresponde ao atual
                    #gera lista html
                    print "<li><b>Matrícula:</b> " k[2] " <b> Total tempo estacionado:</b> " total_por_viatura[key] "</li>"
                }
            }
            print "</ul>"
        }
    }' arquivo-*.park >> stats.html

    echo "</ul>" >> stats.html
}

## <h2>Stats6:</h2>
## <ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...  
## </ul>
## ...
## </ul>


## S3.2.7. Obter uma lista do top3 dos nomes mais compridos de condutores cujas viaturas já estiveram estacionadas no parque (ou que ainda estão estacionadas no parque), ordenados decrescentemente pelo tamanho do nome do condutor:
stats7() {
    echo "<h2>Stats7:</h2>" >> stats.html
    echo "<ul>" >> stats.html
    awk -F: '{
        nome=$4 #armazena nome do condutor
        if (!(nome in len)) {  #se ainda não foi processado
            len[nome]=length(nome) #aramzena o comprimento do nome no array len
        }
    }
    END {
        for (n in len) #para cada nome encontrado
            print len[n] ";" n #imrime comprimento ; nome
    }' arquivo-*.park estacionamentos.txt | sort -t";" -k1,1nr | head -n 3 | cut -d";" -f2 | while read nome; do #ordena por ordem numerica, escolhe os primeiros 3, e extrai apenas o nome
        echo "<li><b> Condutor:</b> $nome</li>"
    done >> stats.html

    echo "</ul>" >> stats.html  
}

## <h2>Stats7:</h2>
## <ul>
## <li><b> Condutor:</b> <Nome do Condutor mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor segundo mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor terceiro mais comprido></li>
## </ul>


## S3.3. Processamento do script:
## S3.3.1. O script cria uma página em formato HTML, chamada stats.html, onde lista as várias estatísticas pedidas.
if [[ $# -eq 0 ]]; then #se tiver 0 arg então realiza todas as stats
    STATS="1 2 3 4 5 6 7"
else
    STATS="$@"
fi


for STAT in $STATS; do
    case "$STAT" in
        1) stats1 ;;
        2) stats2 ;;
        3) stats3 ;;
        4) stats4 ;;
        5) stats5 ;;
        6) stats6 ;;
        7) stats7 ;;
        *) so_error S3.2 "Estatisticas inválidas: $STAT" ;;
    esac
done


echo "</body></html>" >> stats.html

so_success S3.3
## O ficheiro stats.html tem o seguinte formato:
## <html><head><meta charset="UTF-8"><title>Park-IUL: Estatísticas de estacionamento</title></head>
## <body><h1>Lista atualizada em <Data Atual, formato AAAA-MM-DD> <Hora Atual, formato HH:MM:SS></h1>
## [html da estatística pedida]
## [html da estatística pedida]
## ...
## </body></html>
## Sempre que o script for chamado, deverá:
## • Criar o ficheiro stats.html.
## • Preencher, neste ficheiro, o cabeçalho, com as duas linhas HTML descritas acima, substituindo os campos pelos valores de data e hora pelos do sistema.
## • Ciclicamente, preencher cada uma das estatísticas pedidas, pela ordem pedida, com o HTML correspondente ao indicado na secção S3.2.
## • No final de todas as estatísticas preenchidas, terminar o ficheiro com a última linha “</body></html>”

