# Build privado do launcher Android

A entrega 0.3 inclui HUD editável e patch Android separado de descoberta de conexão, além de viewport ajustar/preencher/esticar. O empacotador exige também as quatro novas entradas JNI de touch/display. [Receita de uso e recibo](TOUCH_HUD.md). Mantém o guest `-O2` e não comprova missão corrigida.

Após o primeiro relato do tablet, a versão 0.2 compila guest/host com `-O2`. Não reutilizar o arquivo `-O0` do compile probe para entrega. O APK anterior iniciou, mas apresentou falhas de áudio, desempenho e loading; a atualização ainda requer teste. [Investigação](TABLET_LOADING_INVESTIGATION.md).

A biblioteca completa `libmain.so` foi ligada para Android ARM64 com o arquivo estático real das 29.049 funções originais. O APK experimental foi montado com ela, SDL3 e libc++, assinado e verificado. **Isso não é instalação ou boot Android comprovado.** [Primeiro teste](FIRST_TABLET_TEST.md).

## Entradas e receita

Usar as revisões/ferramentas de `dependencies.lock.json`, preparar SDK34/NDK28.2.13676358/CMake3.31.6 e compilar o guest original com continuações conforme [RECOMPILATION.md](RECOMPILATION.md). A ISO/XBE e os arquivos gerados permanecem em `private/`/`build/`. O APK não incorpora ISO, XBE ou recursos; requer o disco do proprietário.

Preparar e qualificar o tradutor:

```sh
python3 scripts/build_vulkan_reports.py
python3 scripts/build_vulkan_shaders.py --android
```

Com os cabeçalhos e arquivo AArch64 privados já gerados:

```sh
scripts/compile_private_generated.sh \
 build/urban-chaos-user-image/resumable-pass1 \
 build/urban-chaos-user-image/optimized-arm64 \
 android-arm64 build/context-tools-v6/sdk/runtime 2

python3 scripts/build_android_runtime.py \
 --xbe private/disc-check/game/Default.xbe \
 --generated build/urban-chaos-user-image/resumable-pass1 \
 --archive build/urban-chaos-user-image/optimized-arm64/liburban_generated_compile_probe.a \
 --profile config/research/user-xbe-resumable.json \
 --out build/android-runtime

python3 scripts/package_private_launcher.py \
 --native-build build/android-runtime --out build/private-apk
```

Os destinos devem ser novos e ignorados. `--resume` no builder nativo repete apenas uma tentativa falha com as mesmas entradas verificadas; `--rebuild` substitui uma biblioteca anterior do mesmo conjunto, preservando recibos, overlays e logs. Não publicar diretórios de build como artefatos globais. Preservar a keystore debug em armazenamento privado para atualizar sem desinstalar; release não usa essa chave.

O empacotador usa Gradle8.7/AGP8.5.2/Java17 para Java, manifesto e DEX; depois injeta somente as três bibliotecas ARM64 verificadas, notices e assina com build-tools34. O APK intermediário sem bibliotecas não é entregue. Essa composição evita cópias repetidas do arquivo nativo grande no workspace. As bibliotecas são comprimidas e `extractNativeLibs=true`; assinatura e alinhamento são conferidos depois. O fingerprint combina hashes do runtime e de todas as fontes de app usadas; mudança durante o Gradle cancela a entrega.

Os metadados JUnit BOM 5.9.2/5.9.3 ausentes na primeira tentativa foram conferidos contra os arquivos e SHA-256 publicados no Maven Central. Somente seus checksums foram acrescentados ao lock do launcher. A verificação Gradle não foi desabilitada.

## Adaptações e evidências

- SDK MIT `c1f7d60...`, SDL3 `fa2c02bb...`, Black renderer/tradutor MIT `8e9c6fc...`, glslang `a57276bf...`; checkouts preservados.
- `vulkan-android-host.patch` aplica-se sobre o overlay headless já testado, restabelece a superfície SDL Android e liga espera de foreground. As queries precisas próprias e os guards de draws contados permanecem.
- Comparação de bytes no render thread usa memcmp em ARM64; SSE continua no ramo x86. Dispatcher inclui shim vazio somente onde não usa intrinsics. Bionic API29 recebe a declaração de ftruncate e syscall memfd_create necessária aos aliases guest.
- NativeMain utiliza o mesmo perfil/ABI do host Linux, monta o disco validado, inicia o kernel original e bombeia eventos SDL. A Activity do jogo usa processo separado; a saída nativa encerra esse processo, preservando o launcher e os diretórios privados.
- Input SDL é lido no event thread e copiado sob mutex; a thread guest não consome eventos da janela. Toque inicial só fornece A/B/START e dpad. Focus perdido limpa entradas e pausa saída de áudio; present espera foreground.
- Importação SAF em staging, lock contra escritores concorrentes, tamanho/espaço limitados, prefixo integral verificado na retomada, fsync e ativação atômica após validação nativa. Saves/config/cache/logs separados; sem permissão ampla de armazenamento.
- Relatórios JNI/ZIP estão ligados às Activities. `UC-GPU-003` registra vendor/device/API/driver/precise, com phase0 para suporte e phase1 para habilitado; `UC-GPU-004` classifica draw contado não implementável. Operações: 1 geometria, 2 depth target, 3 target, 4 textura, 5 shader, 6 pipeline.

Falhas preservadas: chamada a resolução dinâmica ausente no primeiro host Android, include do entry point ausente na primeira ponte JNI, checksums JUnit ausentes e O_DIRECTORY não exposto na API Java. Foram corrigidas e a compilação/assinatura final passou. Ferramentas opcionais de análise/debug do host e produtos antigos regeneráveis foram removidos para caber no workspace; compilador/linker/headers, entrada original e arquivos guest atuais foram mantidos.

## CI pública

Compila código de interface Android com perfil autoral, renderer com SDL/Vulkan e arquivos do host/dispatcher/render thread em AArch64; compila Java/SDL contra SDK34 e executa testes de importação/relatórios com bytes autorais. Não recebe entradas comerciais nem distribui o APK do jogo. O build privado acima é a evidência de link do título; o contrato público não o substitui. A tentativa Windows legada e gates de release continuam independentes.
