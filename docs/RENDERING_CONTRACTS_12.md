# Renderização: texturas assinadas e amostragem entre passes

Investigação de 2026-11-25. **GO para as correções de contrato abaixo; missão jogável ainda não qualificada.** A imagem real melhorou após corrigir a amostragem, mas ainda não comprova fidelidade da missão, desempenho no Adreno ou campanha. Não há novo APK entregue nesta investigação.

## Erros reproduzidos e corrigidos

1. **D3DTSS_COLORSIGN ignorado.** O jogo vincula cubemaps A8R8G8B8 com RGB assinado (`0xe0000000`) nos passes de iluminação. O estado chegava ao runtime, mas o renderer carregava todos os componentes como UNORM. Agora a conversão para RGBA16_SNORM acontece antes da filtragem, preservando alpha e os componentes não assinados. Interpretar o sinal depois de interpolar não é equivalente: bytes +127/-128 devem interpolar para zero. A mesma memória pode ter variantes assinada e não assinada simultaneamente, com invalidação independente. Formatos/aliases assinados não implementados falham explicitamente; não são tratados silenciosamente como cores comuns.
2. **Meio texel aplicado indiscriminadamente.** Leituras de render target sempre recebiam deslocamento de meio texel, inclusive quando o vertex program escrevia posições de tela diretamente. Isso deslocava/amaciava imagens em passes sucessivos. O backend agora distingue esses programas daqueles que usam a constante de viewport, e acompanha as derivadas das coordenadas para a correção dos targets. Inclui o alias de depth como cor. A mudança não desativa bloom, iluminação, histórico ou qualquer outro pass.
3. **Discard presente com alpha test desligado.** O shader sempre continha um caminho de descarte, mesmo quando o jogo desativava o teste. O estado agora participa da chave de shader e elimina esse caminho quando dispensável, permitindo os testes antecipados de profundidade do driver. Alpha test e clip de textura continuam efetivos quando exigidos. Não se trata de FPS medido no tablet.

Os dois últimos contratos usam backports MIT do XboxXBERecomp: [09f0a371](https://github.com/TGP482/XboxXBERecomp/commit/09f0a371ecc47e4210273c85f6719a9b22e89451) e [7beac8cd](https://github.com/TGP482/XboxXBERecomp/commit/7beac8cd24073281b6a134dfb3de2b591686ae97). Commits, hashes dos patches e aviso de licença estão no lockfile. O pin base permanece `c1f7d60...`; checkout vendor limpo. O conversor de componentes e os testes são próprios. Nenhuma adaptação da Burner sem licença esclarecida foi incorporada.

## Testes que distinguem a falha da correção

- O teste de textura assinada falha no renderer anterior: recebe 255 onde deve receber aproximadamente 128. A correção passa filtragem, máscaras de canais, alpha não assinado, dois estágios sobre o mesmo endereço, cubemap/mips e atualização dentro do mesmo Present.
- Um padrão autoral de alta frequência atravessa dois render targets. Antes da correção, a comparação falha com filtro linear no pixel (1,0). Depois, todos os texels são preservados com filtros nearest e linear.
- O teste de alpha inspeciona o SPIR-V compilado: OpKill só aparece quando o teste está ligado. As fixtures Vulkan existentes continuam verificando rejeição alpha e clip de textura com queries reais.
- A fixture antiga de filtragem do alias de depth usava coordenada 4,5 já contando com o deslocamento incondicional. Foi corrigida para a fronteira real 4,0 entre centros 3,5/4,5, mantendo os mesmos valores esperados de cor/alpha.

Comandos públicos:

```sh
python3 scripts/build_vulkan_renderer.py --android-window
python3 -m unittest discover -s tests -v
python3 scripts/test_diagnostics.py
python3 scripts/build_runtime_probes.py
python3 scripts/publication_check.py
```

Os 66 testes Python, nove probes nativos e diagnóstico Java/JNI passaram. Uma tentativa dos testes de importação encontrou a reserva de disco de 64 MiB durante builds concorrentes; foram liberadas saídas antigas regeneráveis e os testes passaram sem reduzir a guarda.

O builder compila os shaders do overlay correspondente ao header novo; não mistura `DrawConstants` antigo com shader novo. O recibo do runtime Android inclui o hash do conversor. O empacotador recusa uma biblioteca sem esse vínculo ou com hash divergente.

## Execução do jogo e limites da evidência

A investigação privada acompanhou os targets intermediários de um frame completo, incluindo os passes de cor, bloom, curva de cor e histórico. Um segundo diagnóstico comparou profundidade/stencil em ordem, espelhando Clear e superfícies em armazenamento separado. Não é um oráculo de fidelidade: o renderer CPU usa quantização/interpolação e não implementa todos os biases como a GPU; as divergências foram preservadas.

A correção de canais assinados sozinha não resolveu os defeitos. Depois do ajuste de coordenadas, a captura real da abertura de Hope Street apresentou redução visível das faixas serrilhadas e das bordas degradadas. Os ensaios não congelam o mesmo estado de câmera/tempo: não se anuncia métrica numérica de qualidade a partir dessas imagens. Capturas, shaders do jogo, recursos e logs detalhados permanecem privados.

Ainda há sobreposição temporal e regiões muito claras nessa cena. O efeito de histórico do próprio jogo usa peso observado de aproximadamente 0,447; sua presença não prova, sozinha, cache incorreto. Não foi desligado para esconder o problema. Também não se atribui toda a iluminação restante a uma única causa sem comparação de referência.

Um experimento privado com profundidade reversa consistente, incluindo conversão inversa para o alias de cor, não comprovou solução visual. Foi retirado; não integra os patches de produção. Não houve redução de resolução ou troca dos assets.

O ensaio posterior do build limpo durou 300,042 s e chegou à apresentação 2100: 16 reports de visibilidade concluídos, zero pedidos de disco ausentes e nenhuma exceção registrada. As capturas 1920/2040/2100 mostram o interior do veículo, depois arma, mira e HUD nativo em primeira pessoa. Não houve movimento comandado nem missão completa nesse roteiro; não é benchmark do Pad 6. Regiões externas escuras continuam requerendo comparação, sem forçar um aumento artificial de brilho.

A integração ARM64 contém também a união de CFG e os controles já descritos em [MISSION_CONTROLS_11.md](MISSION_CONTROLS_11.md). Compilar essa biblioteca não equivale a instalar um novo APK nem comprova missão jogável. [Recibos desta investigação](evidence/runtime-progress/rendering-contracts-12.json).
