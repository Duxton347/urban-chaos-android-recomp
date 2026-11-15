# Urban Chaos Android — auditoria e execução por gates

## 1. Resultado preliminar e condições

A leitura dos repositórios confirmou dois bloqueios: a ISO disponível é de **PS2**, enquanto a recompilação exige o executável **Xbox**; e a Burner Edition pública declara que ainda depende de entradas privadas para reproduzir seu executável.

**Decisão inicial:** `GO` para infraestrutura e componentes independentes; `BLOCKED` para comprovar a recompilação do jogo. Ainda não há evidência suficiente para determinar se todas as entradas privadas podem ser reconstruídas exclusivamente com ferramentas públicas.

Esta sessão permanece em modo de planejamento. Foram feitas inspeções por leitura; nenhum repositório foi clonado, relatório gravado ou APK compilado.

Versões identificadas para a auditoria:

| Componente | Commit |
|---|---|
| Burner Edition 0.18 | `bdaef722ef1d589af30e5a58dd8543f64564c4fc` |
| Black-Android | `8e9c6fc7d570503ea68d039cff16c09339ee6d30` |
| Runtime pinado pela Burner | `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` |
| Runtime pinado pelo Black | `99078b119b5d5e0d1d6afb1390e29437c0c984c0` |

A compilação será realizada no ambiente remoto Linux; o tablet receberá os APKs para instalação manual. Não depender do computador pessoal quebrado nem presumir acesso ADB ao dispositivo.

## 2. Fase 0 — auditoria reproduzível

- Criar `/workspace/UrbanChaos-Android`, clonar os três projetos em revisões fixas e registrar URLs, hashes, licenças e dependências em lockfile.
- Produzir `AUDIT.md`, `MISSING_INPUTS.md`, `COMPATIBILITY_MATRIX.md` e `PLAN.md` em `docs/`, com evidências, comandos, resultados e condições de cada gate.
- Inventariar as entradas ausentes de `build/xbe-full-integration-20261014`: configuração escaneada, unidades geradas, backups originais, fixtures e `integration-status.json`. Distinguir arquivos indispensáveis ao build daqueles usados somente na validação.
- Rastrear os scripts públicos para identificar como cada entrada é consumida e se existe receita para regenerá-la. Não criar um status de qualificação fictício para ultrapassar as verificações.
- Comparar os runtimes. A inspeção das árvores encontrou diferenças em nove arquivos; os arquivos centrais de contexto CPU, kernel/memória e interfaces HLE permanecem iguais entre esses pins. Usar o pin da Burner como base, mantendo adaptações Android explícitas.
- Auditar a licença de cada arquivo reutilizado. Black e XboxXBERecomp apresentam MIT; o `COPYRIGHT` da Burner não estabelece, sozinho, uma licença geral para suas adaptações próprias. Registrar direitos não esclarecidos e não incorporar esses trechos até haver licença aplicável.

Cada relatório indicará `GO`, `BLOCKED` ou `REQUIRES_REVERSE_ENGINEERING`, sem confundir disponibilidade pública com reprodução comprovada.

## 3. Trabalho independente do jogo

**Ferramentas e interfaces**

- Implementar `inspect_xbe.py` para leitura estrutural, SHA-256, certificado, seções, bibliotecas e importações, sem registrar conteúdo comercial.
- Implementar `prepare_game.py` para validar pasta Xbox ou extrair XDVDFS/XISO; rejeitar imagens PS2 com mensagem específica.
- Implementar `recompile_xbe.py` para executar scanner e recompilador com versões fixas, configuração explícita, logs e geração em diretório ignorado pelo Git.
- Criar scripts Linux de preparação e build. Validar a compilação das ferramentas host isoladamente, sem importar o CMake Windows/D3D12 completo do SDK.

**Android**

- Usar ARM64 exclusivamente, Java 17, Gradle 8.7, AGP 8.5.2, SDK 34, NDK `28.2.13676358`, CMake `3.31.6` e SDL `release-3.4.16`; resolver dependências para commits e checksums no lockfile.
- Criar APK de diagnóstico separado, com SDL3, superfície Vulkan, apresentação de frames de teste e tratamento de suspensão/resumo. Identificá-lo claramente como diagnóstico, sem execução do jogo.
- Implementar importação por SAF para arquivo ou pasta, sem `MANAGE_EXTERNAL_STORAGE`; copiar para armazenamento privado com progresso, cancelamento, retomada e validação antes de ativar a instalação.
- Separar `game`, `saves`, `config`, `cache/shaders` e `logs`. Proteger contra caminhos externos, imagens truncadas, falta de espaço e importações interrompidas.
- Preparar entrada por ações identificadas, persistência do layout e editor de HUD. Vincular ações ao Urban Chaos somente após verificar os consumidores originais.

## 4. Recompilação e integração condicionadas

O manifesto público da Burner identifica este candidato:

- Arquivo: `Default.xbe`
- Tamanho: **5.283.840 bytes**
- SHA-256: `b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac`

Esse hash será uma referência inicial de validação, não uma declaração de compatibilidade Android. O arquivo deverá vir de uma cópia Xbox legitimamente obtida e permanecer fora do Git e dos pacotes de diagnóstico.

**G1 — geração original:** executar o scanner público sobre esse XBE, identificar símbolos ausentes e reconstruir configuração, limites de funções, ABI e hooks com evidências. Gerar novamente os backups a partir de saídas limpas. Se faltarem informações essenciais, registrar os endereços, funções e receitas ausentes e interromper a integração dependente.

**G2–G3 — boot e recursos:** após G1, compilar o código gerado para ARM64; adaptar memória, threads, kernel, arquivos e callbacks. Implementar D3D8→Vulkan e áudio SDL3 respeitando o comportamento observado do Urban Chaos. Não transplantar hooks, estruturas ou endereços de Black. Exigir frames reais até logo/menu.

**G4–G5 — missão, saves e controles:** validar uma missão original, checkpoint, morte, carregamento, cutscenes e saves persistentes. Integrar touch, gamepad, HUD editável e proporção 16:10 sem deformação.

**G6–G7 — desempenho e entrega:** somente após a missão validada, adicionar opções gráficas reversíveis e medir sessões de 15–30 minutos no Pad 6. Entregar APK debug com hash; release exige keystore do proprietário.

## 5. Testes e evidências de entrega

- Ferramentas: XBE inválido/truncado, hash incompatível, extração segura, símbolos não resolvidos e geração determinística.
- APK independente: ELF AArch64, bibliotecas empacotadas, instalação, Vulkan, lifecycle e importação interrompida.
- Jogo: ABI, memória, callbacks, menu, missão, áudio, save/load após encerramento, multitouch e atualização preservando saves.
- Tablet: registrar modelo, Android, driver e capacidades Vulkan reais; testes feitos manualmente serão documentados como tais.

Entregar as instruções de build, importação, HUD, testes e limitações previstas no arquivo original. Cada fase registrará código compilado, teste executado, evidência e próximo bloqueio.

O status distinguirá **compila**, **inicia**, **menu funciona**, **missão jogável** e **campanha verificada**. Sem XBE Xbox e geração reproduzível, a entrega permanecerá limitada aos componentes independentes, com o bloqueio explicitamente documentado.