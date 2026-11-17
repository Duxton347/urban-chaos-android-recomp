# Superfícies retornadas e propriedade de recursos

Investigação de 2026-11-16/17, variante `c6f0c99a…044d1`, SDK `c1f7d60…`.

## Falha comprovada

A repetição em `0x000B9A10` era posterior à corrupção da lista do heap. O rastreamento sem alterar a memória guest encontrou uma alocação retornando o próprio sentinela vivo. Executar o caminho curto do alocador em x86 original, com o estado privado capturado, devolveu o mesmo endereço que o C++ gerado. Isso exclui divergência de tradução naquele caminho específico, não em todo o recompilador.

Uma verificação de reciprocidade dos links, aplicada após retornos de alocação/liberação, encontrou o primeiro rompimento observado após duas liberações do mesmo bloco via `0x00084CB3`/`0x000842D8`: a segunda recebeu o bloco já livre. Não foi removida a árvore, trocado o alocador ou ignorada a segunda liberação.

A busca direcionada no port PC encontrou `scripts/prepare_xbe_resource_refs.py`, que documenta o contrato relevante: um getter de superfície precisa reter também o pai quando cria a primeira referência pública. No SDK pinado, os três getters incrementavam apenas o contador da própria superfície. A rotina original de retain em `0x003E5570` foi conferida independentemente no XBE fornecido: primeiro retain público de superfície preserva o pai; os demais incrementam apenas a própria referência. ABI stdcall com um argumento, `ret 4`; SHA-256 dos 55 bytes originais `fc04f80709305c363d8573abf7ed0d75db798bfb139abc8e23234f38282fd9b5`.

## Correção

`patches/runtime/surface-ownership.patch` faz GetBackBuffer2, GetRenderTarget2 e GetDepthStencilSurface2 chamar o retain original pelo binding revisado `D3D_PublicAddRef`. O helper usa o profile, não um endereço fixo no código do runtime. Leitura e retenção são protegidas pelo mutex recursivo já usado no SDK. Um retorno nulo permanece nulo; a falta do binding falha explicitamente.

A adaptação foi implementada sobre o SDK MIT a partir do contrato original conferido. A implementação e o fixture próprios da Burner não foram incorporados. O estudo do PC orientou a busca e evitou substituir o heap por uma hipótese não comprovada.

## Verificação

O teste público compila os corpos reais dos getters do SDK, com metadata e objetos autorais, e usa CallGuest real para um callback que modela o contrato de retenção:

- Três getters, referência inicial e repetida, substituição do alvo e liberação do proprietário original.
- Pai e superfície sobrevivem enquanto houver um retorno ainda mantido; ambos são destruídos uma única vez após o último release.
- Retorno nulo, preservação dos registradores e balanço de pilha.
- Controle negativo compila os getters do pin original e confirma a ausência do retain do pai. Ele é identificado como controle negativo, não uma correção aprovada.

Na execução privada seguinte, a corrupção/repetição anterior não reapareceu no ponto observado; o título avançou e revelou outra chamada de áudio 3D. Isso não comprova ausência de todos os problemas de ownership nem missão carregada. As provas derivadas do título ficam em diretórios ignorados. [Progresso](RUNTIME_PROGRESS.md) e [regressões executadas](evidence/runtime-progress/surface-spatial-tests.json).
