# Publicação

Destino autorizado: https://github.com/Duxton347/urban-chaos-android-recomp.

Em 2026-11-15 a primeira verificação retornou 404. A tentativa de criação pelo CLI GitHub foi recusada com `Resource not accessible by integration (createRepository)`. O usuário criou o repositório exato, público e vazio. Leitura e permissão de push foram confirmadas antes de qualquer envio. Nenhum destino substituto foi criado e a visibilidade escolhida pelo usuário foi preservada.

Entrega publicada no [PR #1](https://github.com/Duxton347/urban-chaos-android-recomp/pull/1), branch `audit/recompilation-route`, com commits separados para validação de geração, auditoria/arquitetura e workflows. A base anterior foi preservada em um commit inicial; não houve force push nem repositório substituto. Nenhum APK de jogo será publicado nesta etapa. Resultados de CI devem ser conferidos no GitHub e não inferidos de validação local.


## Validações remotas

Os quatro workflows automáticos foram disparados pelo PR no commit `8fb039e3cde9712fe0e44e48812669c3d57bba7d`:

- [Auditoria](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734487605)
- [Host/native smoke](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734487666)
- [Testes](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734487601)
- [Contrato Android ARM64](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734487615)

Artefatos de desenvolvimento exigem dispatch explícito no `main` depois de incorporar a infraestrutura; não foram publicados nesta etapa. O workflow release é uma barreira inicial e falha fechado, não uma release de jogo implementada. Nenhum run recebe conteúdo comercial.

## Resultado confirmado e correção de cache

No commit `4700b93ec19f81589d4bad636ad77269a0807bef`, os quatro workflows automáticos **passaram** após corrigir o bootstrap Android:

- [Auditoria — sucesso](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734725303)
- [Host/native — sucesso](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734725289)
- [Testes — sucesso](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734725352)
- [AArch64 — sucesso](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43734725314)

Na revisão final, o cache inicial de auditoria (`references-*`) foi considerado inadequado por conter checkout Black, cujo upstream inclui `ci/default.xbe.enc`. O cache criado nesta sessão (ID 10559603441) foi excluído pela API e a ausência foi confirmada. A auditoria agora cacheia somente actionlint; o checkout Black usa filtro de blobs/sparse checkout para omitir `ci/` e `*.enc`. Não houve decriptação, incorporação em Git ou publicação desse arquivo como artefato do projeto. Caches de host/NDK contêm apenas as dependências necessárias às suas funções.

As alterações finais de isolamento de checkout/cache são verificadas novamente pelo PR. Consultar os checks do HEAD atual no PR para a execução mais recente; os links acima preservam a evidência do commit explicitamente testado. O PR fica aberto para revisão, sem merge automático.


## Recuperação PC — tentativa real

Commits `e688a49`, `4dc432b`, `76d5d50` adicionaram compilação do núcleo real do SDK, inventário de releases/histórico e reprodução do comando original no Windows. No commit `76d5d50`, os quatro checks de infraestrutura passaram. A [tentativa PC original](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43740071363) **falhou**, com configure exit 0 e build exit 1 por `integration-status.json` ausente. O erro foi preservado; não há build do jogo ou novo APK. [Relatório](PC_BUILD_RECOVERY.md).


## Runtime e continuações — 2026-11-16

A revisão no mesmo PR #1 adiciona correções explícitas MIT de GPU/áudio, geração optativa com continuações, regressões autorais e receita para o host privado. Compilação e execução do título ocorreram somente no ambiente privado da nuvem; o CI público recebe os testes sintéticos GPU/contexto e nenhuma entrada comercial. [Resultados locais e limites](RUNTIME_PROGRESS.md).

O acesso ao destino exato foi reconfirmado antes da publicação. O PR permanece aberto; nenhum merge, release do jogo ou APK foi autorizado como validado. Os checks do novo HEAD serão acompanhados após o push; os sucessos históricos acima não são evidência automática desta revisão.


No commit `482f0120214edb0b8b62a21a4adfb00d6cd41a40`, passaram [auditoria](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43869841739), [testes](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43869841303), [native/contexto](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43869841354) e [contrato ARM64](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43869841375). A [reprodução Windows original](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43869841331) voltou a falhar: configure exit 0, build exit 1, `integration-status.json` privado ausente. O erro foi conferido no log; não foi mascarado.

Esse commit antecede o novo decodificador de pacotes gravados. O novo HEAD acrescenta o teste autoral de índices/retorno ao CI nativo e sua compilação ARM64; seus checks precisam ser acompanhados separadamente. O PR segue aberto, sem merge ou release.
