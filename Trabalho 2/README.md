# Catálogo de filmes

O objetivo deste projeto é aprimorar o anterior por meio da implementação de uma árvore B+ para inserir filmes no catálogo. Apenas complemento do código anterior, porém agora os índices são organizados em uma Árvore B+.

## Sobre o código:

Mesmo estando tudo no mesmo arquivo .c, o código é dividido em partes pelas ED's, a primeira é a árvore B+ e a segunda a lista invertida.
Por fim, temos a parte das operações no arquivo de filme.

**B+:**

- Ordena os índices primários em outro arquivo.
- Tentamos fazer a remoção, mas não conseguimos, então tem apenas o começo da lógica.

**Lista secundária:**

- Usada para ordenar os índices secundários alfabeticamente e ligar os índices secundários aos primários correspondentes.
- Foram utilizados vetores para tal.
- São dois vetores, um de secundários e um com os primários.
