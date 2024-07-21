#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct
{
    char codigo[6]; // chave primaria
    char tituloBr[50];
    char tituloOriginal[50];
    char diretor[40];
    char anoLancamento[5];
    char pais[30];
    int nota;
} Filme;

typedef struct indP
{
    int RRN;
    char chavePrimaria[6];
    struct indP *prox;
} indicePrimario;

typedef struct indS
{
    char titulo[50];
    char chave[100][6]; // vetor para guardar mais chaves caso exista filmes com mesmo titulo
    struct indS *prox;
} indiceSecundario;

// funcao para inserir na lista de indice primario
void criarIndicePrimario(indicePrimario **lista, char *chave, int RRN)
{
    indicePrimario *aux, *novo = malloc(sizeof(indicePrimario));
    strcpy(novo->chavePrimaria, chave);
    novo->RRN = RRN;

    if (*lista == NULL)
    {
        novo->prox = NULL;
        *lista = novo;
    }
    else if (strcmp(novo->chavePrimaria, (*lista)->chavePrimaria) < 0)
    {
        novo->prox = *lista;
        *lista = novo;
    }
    else
    {
        aux = *lista;
        while (aux->prox && strcmp(novo->chavePrimaria, aux->prox->chavePrimaria) >= 0)
            aux = aux->prox;
        novo->prox = aux->prox;
        aux->prox = novo;
    }
}

// funcao para inserir na lista de indice secundario
void criarIndiceSecundario(indiceSecundario **lista, char *chave, char *titulo)
{
    indiceSecundario *aux, *novo = malloc(sizeof(indiceSecundario));
    int i = 0;

    if (*lista == NULL)
    {
        strcpy(novo->chave[0], chave);
        strcpy(novo->titulo, titulo);
        novo->prox = NULL;
        *lista = novo;
        return;
    }

    aux = *lista;
    while (aux)
    {
        if (strcmp(aux->titulo, titulo) == 0) // titulos iguais
        {
            while (aux->chave[i][0] != '\0' && i < 100)
                i++;
            if (i == 100)
                return;
            strcpy(novo->chave[i], chave);
            return;
        }
        aux = aux->prox;
    }

    aux = *lista;
    strcpy(novo->chave[0], chave);
    strcpy(novo->titulo, titulo);

    if (strcmp(novo->titulo, (*lista)->titulo) < 0)
    {
        novo->prox = *lista;
        *lista = novo;
    }
    else
    {
        while (aux->prox && strcmp(novo->titulo, aux->prox->titulo) >= 0)
            aux = aux->prox;
        novo->prox = aux->prox;
        aux->prox = novo;
    }
}

// funcao para alterar a flag quando altero as listas de indices
void mudarFlag()
{
    // abrir o arquivo de indices, se existirem, e sobreescrever a flag
    FILE *ind1 = fopen("iprimary.idx", "r+"); // arquivo de indices primarios
    FILE *ind2 = fopen("ititle.idx", "r+");   // arquivo de indices secundarios
    if (ind1 && ind2)
    {
        fseek(ind1, 0, SEEK_SET); // aponta para o inicio do arquivo, onde esta a flag
        fseek(ind2, 0, SEEK_SET);
        fprintf(ind1, "%d\n", 0);
        fprintf(ind2, "%d\n", 0);
    }
}

// funcao para salvar os indices primarios na RAM, caso exista arquivo consistente
void salvarIndicesPrimarios(FILE *arqInd1, indicePrimario **lista)
{
    // ler o arquivo de indice e ir salvando na lista
    fseek(arqInd1, 0, SEEK_SET);
    int flag;
    fscanf(arqInd1, "%d\n", &flag);
    char linha[30];

    while (fgets(linha, sizeof(linha), arqInd1) != NULL)
    {
        // dividir a linha em chavePrimaria e RRN usando strtok
        char *chavePrimaria = strtok(linha, "@");
        char *RRN = strtok(NULL, "\n");

        if (chavePrimaria != NULL && RRN != NULL)
        {
            int rrn;
            sscanf(RRN, "%d", &rrn);
            criarIndicePrimario(lista, chavePrimaria, rrn);
        }
    }
}

// funcao para refazer os indices primarios na RAM, caso o arquivo seja inconsistente ou nao exista
void refazerIndicesPrimarios(FILE *filmes, indicePrimario **lista)
{
    // ler o arquivo de filmes e salvar a chave primaria e o RRN
    char chavePrimaria[6];
    int RRN = 0;

    fseek(filmes, 0, SEEK_SET);

    while (fgets(chavePrimaria, sizeof(chavePrimaria), filmes))
    {
        chavePrimaria[strlen(chavePrimaria)] = '\0';

        if (chavePrimaria[0] != '*' && chavePrimaria[1] != '|')
            criarIndicePrimario(lista, chavePrimaria, RRN);
        RRN++;
        fseek(filmes, RRN * 192, SEEK_SET);
    }
}

// funcao para salvar os indices secundarios na RAM, caso exista arquivo consistente
void salvarIndicesSecundarios(FILE *arqInd2, indiceSecundario **lista)
{
    // ler o arquivo de indice e ir salvando na lista
    fseek(arqInd2, 0, SEEK_SET);
    int flag;
    fscanf(arqInd2, "%d\n", &flag);
    char linha[60];

    while (fgets(linha, sizeof(linha), arqInd2) != NULL)
    {
        char *titulo = strtok(linha, "@");
        char *chave = strtok(NULL, "\n");

        if (titulo != NULL && chave != NULL)
        {
            criarIndiceSecundario(lista, chave, titulo);
        }
    }
}

// funcao para refazer os indices secundarios na RAM, caso o arquivo seja inconsistente ou nao exista
void refazerIndicesSecundarios(FILE *filmes, indiceSecundario **lista)
{
    // ler o arquivo de filmes e salvar a chave primaria e o titulo
    char chave[6], titulo[50];
    int RRN = 0;

    fseek(filmes, 0, SEEK_SET);

    while (fscanf(filmes, "%5[^@]@%49[^@]", chave, titulo) == 2)
    {
        if (chave[0] != '*' && chave[1] != '|')
            criarIndiceSecundario(lista, chave, titulo);
        RRN++;
        fseek(filmes, RRN * 192, SEEK_SET);
    }
}

// funcao para adicionar um novo filme no arquivo
void escreveArquivo(FILE *filmes, indicePrimario **listap, indiceSecundario **listas)
{
    Filme filme;
    int i;

    printf("\n=================== ADICIONAR FILME ===================\n");
    printf("\tPor favor nao escrever com acentuacao.\n");
    printf("\nTitulo em portugues: ");
    scanf(" %49[^\n]", filme.tituloBr);
    printf("\nTitulo original (se o titulo em portugues for o mesmo, digite 'Idem'): ");
    scanf(" %49[^\n]", filme.tituloOriginal);
    printf("\nNome do diretor [sobrenome, nome]: ");
    scanf(" %39[^\n]", filme.diretor);
    printf("\nAno de lancamento: ");
    scanf("%s", filme.anoLancamento);
    printf("\nPais no qual o filme foi produzido: ");
    scanf(" %29[^\n]", filme.pais);
    printf("\nNota [0-9]: ");
    scanf("%d", &filme.nota);

    // cria o codigo do filme
    // copia as três primeiras letras do sobrenome para a chave e as converte em maiúsculas
    for (i = 0; i < 3 && filme.diretor[i]; i++)
    {
        filme.codigo[i] = toupper(filme.diretor[i]);
    }
    filme.codigo[3] = '\0';

    strncpy(filme.codigo + 3, &filme.anoLancamento[2], 2);
    filme.codigo[5] = '\0';

    // calcular o tamanho variável, somar todos com os delimitadores, fazer 192 - tamanho q eu tenho,
    // e isso que sobrar vai ser a quantidade de # que vou ter que adicionar no final

    // calcula o tamanho total dos campos de texto variável
    size_t tamanhoCamposVariaveis = strlen(filme.tituloBr) + strlen(filme.tituloOriginal) + strlen(filme.diretor) + strlen(filme.pais);

    // calcula a quantidade de "#" necessária para preencher o espaço adicional
    // total 192 bytes = 175 bytes dos campos variáveis + 7 bytes dos separadores (@) + 10 bytes de campos fixos
    size_t quantidadeHashtags = 175 - tamanhoCamposVariaveis;

    // cria uma string preenchida com "#" com base na quantidade calculada
    char espacoAdicional[176];                        // +1 para o caractere nulo no final
    memset(espacoAdicional, '#', quantidadeHashtags); // preenche o vetor
    espacoAdicional[quantidadeHashtags] = '\0';       // adiciona o caractere nulo no final

    // escreve no arquivo
    fprintf(filmes, "%s@%s@%s@%s@%s@%s@%d@%s", filme.codigo, filme.tituloBr, filme.tituloOriginal, filme.diretor, filme.anoLancamento, filme.pais, filme.nota, espacoAdicional);

    // ATUALIZAR NOS INDICES
    // adiciono os indices primarios e secundarios nas listas
    criarIndicePrimario(listap, filme.codigo, ftell(filmes) / 192);
    criarIndiceSecundario(listas, filme.codigo, filme.tituloBr);
    // mudar a flag (sobreescrever o inicio do arq de indices: flag == 0)
    mudarFlag();

    fclose(filmes);
}

void inserir(FILE *filmes, indicePrimario **listap, indiceSecundario **listas)
{
    // verifica se o arquivo de dados existe, se existir abre para leitura e escrita (r+)
    if (filmes)
    {
        // adiciona filme no arquivo de filmes
        fseek(filmes, 0, SEEK_END);             // aponta para o fim do arquivo
        escreveArquivo(filmes, listap, listas); // escreve no fim do arquivo e atualiza os indices
    }
    else
    { // se nao existir, cria o arquivo e abre para leitura e escrita (a+)
        filmes = fopen("movies.dat", "a+");
        if (filmes)
        {
            escreveArquivo(filmes, listap, listas);
        }
        else
        {
            printf("\nErro ao abrir arquivo de filmes.\n\n");
        }
    }
    return;
}

// funcao para buscar indice primario na lista
indicePrimario *buscarPrimario(indicePrimario **lista, char *chave)
{
    indicePrimario *aux = *lista;
    while (aux)
    {
        if (strcmp(aux->chavePrimaria, chave) == 0)
        {
            return aux;
        }
        aux = aux->prox;
    }
    return NULL;
}

// funcao para buscar indices secundario pelo titulo
indiceSecundario *buscarSecundario(indiceSecundario **lista, char *titulo)
{
    indiceSecundario *aux = *lista;
    while (aux)
    {
        if (strcmp(aux->titulo, titulo) == 0)
        {
            return aux;
        }
        aux = aux->prox;
    }
    return NULL;
}

// funcao para remover o indice primario da lista
void removerIndicePrimario(indicePrimario **lista, char *chave)
{
    indicePrimario *aux = buscarPrimario(lista, chave);

    if (aux == NULL)
    {
        printf("Erro ao remover.\n");
        return;
    }
    else if (strcmp(aux->chavePrimaria, (*lista)->chavePrimaria) == 0)
    {
        *lista = (*lista)->prox;
        free(aux);
    }
    else
    {
        indicePrimario *aux2 = *lista;
        while (strcmp(aux->chavePrimaria, aux2->prox->chavePrimaria) != 0)
            aux2 = aux2->prox;
        aux2->prox = aux->prox;
        free(aux);
    }
}

// funcao para buscar o indice secundario
indiceSecundario *buscarIndiceSecundario(indiceSecundario **lista, char *chave)
{
    indiceSecundario *aux = *lista;
    int i;

    while (aux)
    {
        i = 0;

        while (i < 100 && aux->chave[i][0] != '\0')
        {
            if (strcmp(aux->chave[i], chave) == 0)
            {
                return aux;
            }
            i++;
        }

        aux = aux->prox;
    }

    return NULL;
}

// funcao para remover indice secundario da lista
void removerIndiceSecundario(indiceSecundario **lista, char *chave)
{
    indiceSecundario *aux = buscarIndiceSecundario(lista, chave);

    int cont = 0, i = 0, j = 0;

    if (aux == NULL)
    {
        printf("Erro ao remover.\n");
    }
    else
    {
        while (i < 100)
        {
            if (aux->chave[i][0] != '\0')
            {
                cont++;
            }
            if (cont > 1)
            { // tem mais de uma chave
                while (strcmp(aux->chave[j], chave) != 0)
                    j++;
                aux->chave[j][0] = '\0';
            }
            i++;
        }
        while (strcmp(aux->chave[j], chave) != 0)
            j++;
        aux->chave[j][0] = '\0';
        if (cont == 1) // se so tiver uma chave, quer dizer que so tenho 1 filme com aquele titulo, entao removo o no da lista
        {
            if (aux == *lista)
            {
                *lista = (*lista)->prox;
                free(aux);
            }
            else
            {
                indiceSecundario *aux2 = *lista;
                while (strcmp(aux->chave, aux2->prox->chave) != 0)
                    aux2 = aux2->prox;
                aux2->prox = aux->prox;
                free(aux);
            }
        } // se eu tiver mais de uma chave, quer dizer que tenho varios filmes com o mesmo titulo, entao nao posso remover o no, se nao estaria apagando todos os filmes com o mesmo titulo
    }
}

// funcao para localizar o registro no arquivo de filmes e marcar como removido
void removerArquivoFilmes(FILE *filmes, int RRN)
{
    // calcular a posicao do filme no arquivo
    long posicao = (long)RRN * 192;

    // move o ponteiro do arquivo para a posicao desejada
    fseek(filmes, posicao, SEEK_SET);
    // marcar como removido
    char marcacao[] = "*|";
    // escreve os caracteres na posicao do registro
    fprintf(filmes, "%s", marcacao);
}

// funcao para remover filme a partir da chave primaria
void removerFilme(indicePrimario **listap, indiceSecundario **listas, FILE *filmes)
{
    char chave[6];

    printf("\n==================== REMOVER FILME =====================\n");
    printf("Informe a chave primaria do filme a ser removido: ");
    scanf("%s", chave);
    indicePrimario *removerChave = buscarPrimario(listap, chave);
    if (removerChave) // se encontrar a chave do filme que quero retirar
    {
        // pegar o RRN do indice encontrado e localizar no arquivo de filmes para colocar "*|"
        removerArquivoFilmes(filmes, removerChave->RRN);
        // remover da lista de indices
        removerIndicePrimario(listap, chave);
        indiceSecundario *remover = buscarIndiceSecundario(listas, chave);
        if (remover)
        {
            removerIndiceSecundario(listas, chave);
        }
        mudarFlag(); // atualiza a flag
    }
    else
    {
        printf("\nChave nao encontrada.\n\n");
    }
}

// funcao para ler o arquivo de filmes e imprimir na tela
void lerArquivoFilmes(char linha[])
{
    Filme f;
    char *token;

    token = strtok(linha, "@"); // primeiro token (codigo)
    if (token != NULL)
    {
        strcpy(f.codigo, token);
    }

    if (token != NULL && strstr(f.codigo, "*|") == NULL)
    {
        token = strtok(NULL, "@"); // segundo token (titulo em portugues)
        if (token != NULL)
        {
            strcpy(f.tituloBr, token);
        }

        token = strtok(NULL, "@"); // terceiro token (titulo original)
        if (token != NULL)
        {
            strcpy(f.tituloOriginal, token);
            if (strcmp(f.tituloOriginal, "Idem") == 0)
            {
                strcpy(f.tituloOriginal, f.tituloBr);
            }
        }

        token = strtok(NULL, "@"); // quarto token (diretor)
        if (token != NULL)
        {
            strcpy(f.diretor, token);
        }

        token = strtok(NULL, "@"); // quinto token (ano de lancamento)
        if (token != NULL)
        {
            strcpy(f.anoLancamento, token);
        }

        token = strtok(NULL, "@"); // sexto token (pais de origem)
        if (token != NULL)
        {
            strcpy(f.pais, token);
        }

        token = strtok(NULL, "@"); // setimo token (nota)
        if (token != NULL)
        {
            sscanf(token, "%d", &f.nota);
        }
        // escrevo os dados do filme lido
        if (token != NULL)
        {
            printf("\nTitulo em portugues: %s\n", f.tituloBr);
            printf("Titulo original: %s\n", f.tituloOriginal);
            printf("Diretor: %s\n", f.diretor);
            printf("Ano de lancamento: %s\n", f.anoLancamento);
            printf("Pais de origem: %s\n", f.pais);
            printf("Nota: %d\n", f.nota);
            printf("\n");
        }
    }
}

// funcao para buscar o filme pelo RRN no arquivo de filmes
void buscarFilme(FILE *filmes, int RRN)
{
    char codigo[6];
    long posicao = (long)RRN * 192;
    fseek(filmes, posicao, SEEK_SET); // encontra o filme desejado no arquivo
    fscanf(filmes, "%6[^@]@", codigo);

    if (codigo[0] == '*' && codigo[1] == '|') // filme removido
    {
        printf("Filme nao encontrado.\n\n");
        return;
    }
    else
    {
        fseek(filmes, posicao, SEEK_SET);
        char linha[192];
        fgets(linha, sizeof(linha), filmes);
        lerArquivoFilmes(linha);
    }
}

// funcao para buscar filme a partir da chave primaria
void buscarFilmeChave(indicePrimario **lista, FILE *filmes)
{
    char chave[6];
    printf("\n===================== BUSCAR FILME =====================\n");
    printf("Informe a chave primaria do filme: ");
    scanf("%s", chave);
    // busca a chave na lista de indices primarios
    indicePrimario *filme = buscarPrimario(lista, chave);
    if (filme) // se encontrar, localizar no arquivo de filmes e imprimir os dados
    {
        buscarFilme(filmes, filme->RRN);
    }
    else
    {
        printf("\nChave nao encontrada.\n\n");
    }
}

// funcao para buscar filme pelo titulo
void buscarTitulo(indiceSecundario **listas, indicePrimario **listap, FILE *filmes)
{
    char titulo[50];
    scanf("%*c"); // limpa o buffer
    printf("\n===================== BUSCAR FILME =====================\n");
    printf("Informe o titulo do filme em portugues: ");
    scanf("%50[^\n]", titulo);

    /*
    buscar na lista de indices secundarios o titulo, se achar vou ter a chave,
    uso a chave para buscar na lista de indices primarios, se achar vou ter o RRN,
    uso o RRN para localizar o filme no arquivo de filmes, verifico se existe e printo.
    */

    // fazer uma funcao para buscar o indice secundario a partir do titulo (buscarSecundario())
    indiceSecundario *ind2 = buscarSecundario(listas, titulo);
    if (ind2) // se achar o filme na lista de indice secundario
    {
        int i = 0;
        // procurar o filme pela chave na lista de indice primario
        while (i < 100)
        {
            if (ind2->chave[i][0] != '\0')
            {
                indicePrimario *ind1 = buscarPrimario(listap, ind2->chave[i]);
                if (ind1)
                {
                    // uso o RRN para localizar o filme no arquivo de filmes
                    buscarFilme(filmes, ind1->RRN);
                }
                else
                {
                    printf("\nFilme nao encontrado.\n\n");
                }
            }
            i++;
        }
    }
    else
    {
        printf("\nFilme nao encontrado.\n\n");
    }
}

// funcao para localizar e alterar o campo nota no arquivo de filmes
void mudaNotaArquivo(FILE *filmes, int RRN)
{
    int novaNota;
    // calcular a posicao do filme no arquivo
    long posicao = (long)RRN * 192;
    // move o ponteiro do arquivo para a posicao desejada
    fseek(filmes, posicao, SEEK_SET);
    // procura pelo sexto @ para chegar ao campo nota
    int contSeparador = 0;
    char caractere;
    while (contSeparador < 6 && (caractere = fgetc(filmes)) != EOF)
    {
        if (caractere == '@')
        {
            contSeparador++;
            if (contSeparador == 6)
            {
                printf("\nNova nota: ");
                scanf("%d", &novaNota);
                fseek(filmes, 0, SEEK_CUR);
                fprintf(filmes, "%d", novaNota);
            }
        }
    }
}

// funcao para alterar a nota de um filme
void modificarNota(indicePrimario **lista, FILE *filmes)
{
    char chave[6];
    printf("\n==================== ALTERAR NOTA ====================\n");
    printf("Informe a chave primaria do filme: ");
    scanf("%s", chave);
    indicePrimario *modificarFilme = buscarPrimario(lista, chave);
    if (modificarFilme) // se encontrar, localizar no arquivo e alterar a nota
    {
        mudaNotaArquivo(filmes, modificarFilme->RRN);
    }
    else
    {
        printf("\nChave nao encontrada.\n\n");
    }
}

// funcao para listar todos os filmes no catálogo
void listarTodosFilmes()
{
    FILE *filmes = fopen("movies.dat", "r"); // abre arquivo de filmes para leitura

    if (filmes)
    { // se existir o arquivo e for aberto corretamente, listo os filmes
        printf("\n=================== LISTA DE FILMES ====================\n");
        char linha[192];
        while (fgets(linha, sizeof(linha), filmes) != NULL)
        { // enquanto nao chegar ao final do arquivo
            lerArquivoFilmes(linha);
        }
        fclose(filmes);
    }
    else
    { // arquivo nao existe ou erro ao abrir
        printf("\nArquivo não existe.\n");
    }
}

// funcao para escrever no arquivo de indices primarios
void escreverPrimario(indicePrimario **lista, FILE *arq)
{
    fseek(arq, 0, SEEK_SET);
    fprintf(arq, "%d\n", 1); // flag = 1
    while (*lista != NULL)
    {
        fprintf(arq, "%s@%d\n", (*lista)->chavePrimaria, (*lista)->RRN);
        *lista = (*lista)->prox;
    }
}

// funcao para escrever no arquivo de indices secundarios
void escreverSecundario(indiceSecundario **lista, FILE *arq)
{
    fseek(arq, 0, SEEK_SET);
    fprintf(arq, "%d\n", 1); // flag = 1
    indiceSecundario *aux = *lista;
    while (aux != NULL)
    { // enquanto a lista nao chegar ao fim
        fprintf(arq, "%s@%s\n", aux->titulo, aux->chave);
        aux = aux->prox;
    }
}

// funcao para gravar nos arquivos de indice e finalizar o programa
void finalizar(indicePrimario **listap, indiceSecundario **listas)
{

    FILE *ind1 = fopen("iprimary.idx", "r+"); // arquivo de indices primarios
    FILE *ind2 = fopen("ititle.idx", "r+");   // arquivo de indices secundarios

    if (ind1 && ind2) // se os arquivos de indice ja existirem, atualizar os arquivos
    {
        // vou ler a flag de ambos os arquivos, se for 0, então reescrevo, se for 1, nao faco nada pois o arquivo esta correto
        int flag1, flag2;
        fscanf(ind1, "%d\n", &flag1);
        if (flag1 == 0)
        {
            escreverPrimario(listap, ind1);
        }

        fscanf(ind2, "%d\n", &flag2);
        if (flag2 == 0)
        {
            escreverSecundario(listas, ind2);
        }
    }
    else // se os arquivos nao existirem, vou criar e escrever
    {
        FILE *ind1 = fopen("iprimary.idx", "a+"); // arquivo de indices primarios
        FILE *ind2 = fopen("ititle.idx", "a+");   // arquivo de indices secundarios
        if (ind1 && ind2)
        {
            // escrever as listas de indices nos respectivos arquivos
            escreverPrimario(listap, ind1);
            escreverSecundario(listas, ind2);
        }
        else
        {
            printf("\nErro ao abrir arquivo.\n\n");
        }
    }
}

void menu(indicePrimario **listap, indiceSecundario **listas)
{
    // verifica se o arquivo de dados existe, se existir abre para leitura e escrita (r+)
    FILE *filmes = fopen("movies.dat", "r+");

    // quando abrir o arquivo de indices e eles existirem e tiverem com a flag 1, então leio e salvo na RAM
    // quando abrir o arq de indices e a flag for 0, entao refazer a lista na RAM e depois no final do prog vou salvar no arquivo
    // para refazer a lista na RAM vou ter que ler meu arquivo de filmes e ir criando as listas de indices
    // se os arquivos de indice nao existirem, criar os indices na lista e só no final do programa que vou criar os arquivos e gravar
    int op, n;

    do
    {
        printf("========================================================\n");
        printf("Escolha uma opcao:\n");
        printf("1 - Inserir novo filme\n2 - Remover filme\n3 - Buscar filme\n4 - Modificar nota de filme\n5 - Listar todos os filmes\n6 - Sair\n");
        printf("========================================================\n");
        scanf("%d", &op);
        switch (op)
        {
        case 1:
            inserir(filmes, listap, listas);
            break;
        case 2:
            if (filmes) // so vai remover se existir arquivo de filmes
            {
                removerFilme(listap, listas, filmes);
            }
            else
            {
                printf("\nErro, arquivo nao existe.\n\n");
            }
            break;
        case 3:
            if (filmes) // so vai buscar se existir arquivo de filmes
            {
                do
                {
                    printf("========================================================\n");
                    printf("Escolha uma opcao:\n");
                    printf("1 - Buscar pelo titulo em portugues\n2 - Buscar pela chave\n3 - Sair\n");
                    printf("========================================================\n");
                    scanf("%d", &n);
                    switch (n)
                    {
                    case 1:
                        buscarTitulo(listas, listap, filmes);
                        break;
                    case 2:
                        buscarFilmeChave(listap, filmes);
                        break;
                    case 3:
                        break;
                    default:
                        printf("Opção inválida.\n");
                    }

                } while (n != 3);
            }
            else
            {
                printf("\nErro, arquivo nao existe.\n\n");
            }
            break;
        case 4:
            if (filmes) // so vai modificar nota se o arquivo existir
            {
                modificarNota(listap, filmes);
            }
            else
            {
                printf("\nErro, arquivo nao existe.\n\n");
            }
            break;
        case 5:
            if (filmes) // so vai listar se o arquivo de filmes existir
            {
                listarTodosFilmes();
            }
            else
            {
                printf("\nErro, arquivo nao existe.\n\n");
            }
            break;
        case 6:
            break;
        default:
            printf("\nOpção inválida.\n");
        }
    } while (op != 6);

    if (op == 6)
    {
        finalizar(listap, listas);
        printf("Finalizando...\n");
    }

    fclose(filmes);
}

int main()
{
    // criar as listas de indices primario e secundario
    indicePrimario *listap = NULL;
    indiceSecundario *listas = NULL;

    // verifica se o arquivo de dados existe, se existir abre para leitura e escrita (r+)
    FILE *filmes = fopen("movies.dat", "r+");

    // verifica se o arquivo de indices existe, se existirem abre para leitura e escrita (r+)
    FILE *ind1 = fopen("iprimary.idx", "r+");
    FILE *ind2 = fopen("ititle.idx", "r+");

    if (filmes) // se existir arquivo de filmes
    {
        if (ind1 && ind2) // se existirem arquivos de indices
        {
            int flag1, flag2;
            // verificar se os arquivos estao consistentes (flag = 1)
            fscanf(ind1, "%d\n", &flag1);
            if (flag1 == 1)
            {
                salvarIndicesPrimarios(ind1, &listap); // salva indices na RAM
            }
            else
            {
                refazerIndicesPrimarios(filmes, &listap); // refaz indices na RAM
            }

            fscanf(ind2, "%d\n", &flag2);
            if (flag2 == 1)
            {
                salvarIndicesSecundarios(ind2, &listas); // salva indices na RAM
            }
            else
            {
                refazerIndicesSecundarios(filmes, &listas); // refaz indices na RAM
            }
            fclose(ind1);
            fclose(ind2);
        }
        else
        {
            refazerIndicesPrimarios(filmes, &listap);
            refazerIndicesSecundarios(filmes, &listas);
        }
        fclose(filmes);
    }
    else
    {
        printf("\nArquivo de filmes nao existe.\n\n");
    }

    menu(&listap, &listas);

    return 0;
}