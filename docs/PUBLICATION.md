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
