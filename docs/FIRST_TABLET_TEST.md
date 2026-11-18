# Primeiro APK experimental — teste no tablet

**Atualização:** a primeira entrega foi instalada e iniciou, mas a missão permaneceu em loading e vídeo/áudio apresentaram travamentos. Usar agora a **versão 0.2**, build `experimental-16fce2d9b3011686`, APK 28.178.216 bytes, SHA-256 `991c26e2c94a17895a146c4ca9f0cccb0e9c11256822fa83ceabad428a2611e9`. Instalar por cima, sem desinstalar/limpar dados ou repetir a importação. Guest/host otimizados e novas medições; correção da missão e ganho de FPS ainda não comprovados. Se continuar no loading, aguardar 60 s e exportar novamente. [Análise do relato e testes](TABLET_LOADING_INVESTIGATION.md). Os dados da primeira entrega abaixo são históricos.

Build privado de 2026-11-18: `experimental-3ef6e0e968626f62`.

- APK: `UrbanChaos-experimental-arm64.apk`, 45.446.952 bytes (43,3 MiB).
- SHA-256: `9d06ba4b9e59479e0e41d1aac2d31622a1bdcfede5c06a9c962a645c84922aba`.
- Pacote: `org.urbanchaos.launcher`; Android mínimo API 29; ARM64 exclusivamente; assinatura debug.
- O APK contém a biblioteca real do jogo recompilado, SDL3 e libc++, sem ISO, XBE ou recursos do disco. Contém código derivado do executável fornecido: **destinado ao teste privado do proprietário; não publicar como release/artefato público sem a revisão de direitos prevista no projeto**.

**Compila, liga e está assinado/verificado. Instalação, boot/menu, Vulkan/áudio no Pad 6, missão jogável e campanha ainda não foram testados.** O menu e a abertura 3D já observados pertencem ao host Linux, não ao tablet. Não há um resultado Android fictício substituindo esses testes.

## Instalação e primeira execução

1. Baixar o APK entregue privadamente nesta sessão e instalar manualmente no Pad 6. O Android poderá pedir autorização de instalação ao aplicativo usado para abrir o arquivo. Não é necessário PC ou ADB.
2. Abrir **Urban Chaos Experimental** e selecionar a própria ISO Xbox **descompactada** pelo seletor Android. ZIP/RAR/7z e imagens PS2 não são entradas suportadas. Nesta primeira versão, a UI importa ISO; seleção de pasta fica para etapa posterior.
3. A ISO é copiada para armazenamento privado, sem extrair todos os recursos, e validada antes da ativação. É necessário espaço para outra cópia da ISO e margem de 64 MiB. Providers sem tamanho de arquivo conhecido devem ser substituídos por um arquivo salvo localmente no tablet.
4. A cópia pode ser pausada e retomada. A retomada verifica todos os bytes já escritos antes de continuar; não substitui a instalação ativa enquanto estiver incompleta. Se o provider retirar a permissão após reinício, selecionar novamente a mesma ISO. Uma origem diferente falha ao conferir o prefixo. **Descartar importação pendente** preserva a origem, instalação ativa e saves.
5. Tocar **Iniciar jogo experimental**. O alvo inicial do teste é aparecer o vídeo/menu original. Os botões A/B/START e direcionais são entradas Xbox de menu; não constituem o HUD completo de gameplay. O caminho de gamepad SDL está compilado, mas Bluetooth/USB ainda exigem teste real. Não há rumble implementado nesta versão.
6. Testar sair para a tela inicial do Android e voltar. Anotar se o vídeo e áudio retomam, se há travamento ou comandos presos. A pausa do render/áudio está ligada; a suspensão de todas as threads/relógios guest ainda requer validação.
7. Usar **Exportar relatório**. Se o processo do jogo fechar, reabrir o launcher e exportar por ele; as sessões recentes são incluídas. Enviar o ZIP e informar o último passo visível: importação, vídeo, menu, carregamento ou cena 3D.

O relatório inclui build, modelo/API/ABI, tamanho de página, versão/IDs do dispositivo Vulkan, suporte/habilitação de queries precisas, frames e falhas classificadas quando disponíveis. Não inclui a ISO, saves, PCM, imagens, memória ou logs brutos. `session.open` indica ausência de encerramento limpo, não prova isolada de crash.

## Limites atuais

- Somente a revisão compilada é aceita: Default.xbe SHA-256 `c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1`. Revisão diferente é informada e não ativada; a instalação anterior é preservada. Não há recompilação no tablet.
- O alocador guest atual exige páginas host de 4 KiB. O app consulta o valor real e registra `UC-ANDROID-002` se não atender. Alinhamento ELF de 16 KiB não significa que o mapeamento guest já suporte host de 16 KiB.
- Cache de shaders e resolução dinâmica permanecem desativados durante qualificação. Preservam-se o aspecto 4:3, resolução interna inicial do jogo, fog e bloom. Fidelidade visual/sonora e desempenho não estão aprovados.
- Fences/busy de recursos, APIs restantes, missão completa, checkpoint/morte/load, saves após reabrir e atualização preservando progresso ainda precisam de teste. Armazenar a ISO ativa e configurações não prova que o save original funciona.
- HUD completo, editor/perfis, câmera e ações específicas de escudo/equipe por toque continuam pendentes. Sem promessa de FPS/resolução no tablet.

O APK não está publicado no GitHub. O repositório contém fonte, receitas e metadados sanitizados; os binários com código do título ficam no workspace privado. [Verificações de construção](evidence/runtime-progress/android-launcher-apk.json).
