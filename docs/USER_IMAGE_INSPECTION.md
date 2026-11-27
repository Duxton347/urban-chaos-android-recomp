# Imagem Xbox fornecida — inspeção e execução real

> Histórico desta etapa. O estado posterior, incluindo menu original e novas correções, está em [RUNTIME_PROGRESS.md](RUNTIME_PROGRESS.md).

> Relatório da primeira execução. A [investigação posterior de áudio](AUDIO_INVESTIGATION.md) passou pela falha inicial com patch experimental e encontrou uma falha gráfica. Windows é opcional para a rota Android.

Em 2026-11-16 a imagem indicada pelo usuário foi acessada na nuvem. **O `Default.xbe` foi encontrado e extraído.** A falta desse arquivo deixou de ser o bloqueio. O código original foi regenerado, compilado e ligado a um host de pesquisa Linux; a execução ainda falha antes de comprovar menu ou frames. Não há APK jogável nem boot Windows demonstrado.

## O que a imagem contém

A origem respondeu como ISO/XDVDFS direta de **4.050.780.160 bytes**, com partição na base zero, 192 arquivos e dois diretórios. O acesso usado não exigiu descompactação de ZIP/RAR. O executável está na raiz lógica XDVDFS; ferramentas que mostram somente o sistema de arquivos de CD/DVD podem não exibi-lo.

Foram usadas leituras HTTP por intervalo para evitar baixar os 4 GB completos. O executável tem **5.283.840 bytes**, SHA-256 `c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1`, Title ID `534300F7`, versão de certificado `3`, XDK `5849`, 17 seções e 146 imports kernel. As 17 verificações de digest de seção passaram. Isso não autentica a assinatura RSA nem a imagem inteira.

Esse SHA-256 **difere** do candidato Burner `b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac`. Tamanho, identidade e oito testemunhos de bytes dos preparadores públicos coincidem. Os 190 caminhos exigidos pelo manifesto Burner também existem com os tamanhos previstos. A causa da diferença entre executáveis ainda não foi determinada; esses testes não autorizam aplicar todos os patches/endereço da referência automaticamente.

## Etapas executadas

| Etapa | Resultado observado |
|---|---|
| Scanner público no pin `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` | Exit 0; 238 símbolos HLE, 188 com endereço e 50 com zero; nenhuma entrada ABI específica emitida |
| Recompilador público, duas execuções independentes | Ambas exit 0; 29.049 funções e 1.249.894 instruções decodificadas; 132 arquivos byte a byte iguais |
| Compilação C++ real Linux x86_64, GNU 14 | Exit 0; 128 partições e mapping, 129 objetos no arquivo estático privado |
| Ligação com harness original do SDK | Exit 0 após corrigir flag de CPU do host para `-march=x86-64-v2` |
| Primeira execução com logging | Inicialização de dispositivo D3D 640×480 e criação de 64 vertex shaders; timeout em 12 segundos; nenhum frame comprovado |
| Recursos locais | 79 arquivos LST, 10.202.352 bytes, todos SHA-256 iguais ao manifesto público |
| Execução com os 79 LST disponíveis | Exit 139 / SIGSEGV, antes de menu; backtrace situa falha na função guest `0x00423B41` |

A última função corresponde ao export DSOUND `0xB0036`, que o próprio SDK avisou que continuava executando código Xbox em vez de HLE. O endereço host inválido observado foi `0x100000004`. A relação entre objetos de áudio do host, os wrappers não reconhecidos e a chamada que falhou precisa ser rastreada; o backtrace localiza a falha, mas não demonstra sozinho a correção necessária. Não foram criados stubs ou retornos artificiais para passar essa etapa.

O loader também informou uma implementação kernel ausente: ordinal 232, `NtUserIoApcDispatcher`. Não foi estabelecido se ela foi chamada no teste. Os 50 símbolos com endereço zero podem ser ausentes da imagem ou não reconhecidos; não se presume que todos sejam indispensáveis.

O host usa os backends de teste sem apresentação/áudio do harness SDK. Seu descritor `GameInfo` contém somente metadados; não substitui funções do título. Um observador de sinal foi adicionado apenas ao experimento privado para obter a pilha. Essa execução comprova entrada em código recompilado e inicialização parcial; não comprova renderização, áudio, Windows ou Android.

## Reprodução privada das etapas públicas

Guardar o link em `private/shared-link.txt`, sem incluí-lo no Git, e executar em ambiente com acesso à origem:

```bash
python3 tools/inspect_disc.py --drive-link-file private/shared-link.txt \
  --extract-default private/disc-check/Default.xbe \
  --report private/disc-check/inspection.json
build/host/xbe_symbols private/disc-check/Default.xbe vendor/runtime/xdk \
  --out private/disc-check/config/scanned.toml --name urban_chaos
python3 tools/recompile_xbe.py private/disc-check/Default.xbe \
  --config private/disc-check/config/scanned.toml \
  --expected-sha256 c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1 \
  --out build/private-generation
scripts/compile_private_generated.sh build/private-generation/pass1 build/private-objects
```

O destino da extração deve ser novo. O wrapper exige o SHA explícito da variante de pesquisa, mantém a referência original por padrão e não aprova G1. Conferir o caminho gerado no recibo antes da compilação. As duas gerações desta inspeção foram executadas diretamente pelo CLI pinado, com `--xdk vendor/runtime/xdk`, em `build/urban-chaos-user-image/pass1` e `pass2`. O host experimental e seus logs permanecem privados; ainda não há uma receita de host de produção validada.

## Próximo trabalho e limites

1. Rastrear a chamada DSOUND que falha, identificar os wrappers realmente usados, seus objetos e ABI; ajustar reconhecimento/integração com evidência e testes.
2. Revisar callbacks, limites e símbolos da variante fornecida; não transplantar endereços Black ou aprovar `tools_qualified` fictício.
3. Disponibilizar privadamente os demais recursos conforme forem necessários. Filmes, áudio e níveis completos ainda não foram extraídos nem tiveram conteúdo integral validado.
4. Validar boot e menu em host com backend gráfico real; depois prosseguir com ARM64/Vulkan. O build original Burner Windows continua sem sua área privada de integração, e a licença geral de suas adaptações permanece não esclarecida.

Metadados sanitizados: [inspection.json](evidence/user-image/inspection.json). Não foram publicados link/tokens, imagem, XBE, assets, código gerado, binário derivado ou logs brutos do jogo. Não há hash SHA-256 da ISO inteira. Testes públicos utilizam dados sintéticos; 46 testes Python e 11 checks Java passaram localmente.
