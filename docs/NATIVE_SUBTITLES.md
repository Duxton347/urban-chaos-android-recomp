# Legendas nativas e controles — investigação 0.4

## Estado comprovado

O relato `jogo abriu.zip` confirma entrada na missão no tablet após a abertura; não comprova missão jogável ou campanha. A versão 0.3 apresentou lentidão grave ao entrar em 3D. O teste Linux limpo também chegou à cena externa inicial de Hope Street; software Vulkan não mede desempenho Adreno.

A navegação por analógico esquerdo foi executada no jogo original até **Options → Controller**. Essa tela já contém **Horizontal Sensitivity**, **Vertical Sensitivity**, inversão vertical, auto-centre e configuração dos analógicos. Não foi criado um segundo menu de sensibilidade. O modo de navegação do HUD agora usa o analógico esquerdo; o modo Xbox completo mantém dois analógicos, D-pad, gatilhos e demais botões. A migração do layout anterior preserva os cinco botões comuns e acrescenta o joystick padrão.

## Localização dos textos

Leitores próprios identificaram os bancos `globals.exttxt` em `globallanguage.lst`, `99-01.exttxt` em `99-01.ent` e `12-01.exttxt` em `12-01.ent`. O último contém textos/legendas de Hope Street. Não foi copiada a implementação de patches da Burner.

Os contêineres têm índice de recursos, streams zlib, setores de 2048 bytes e trailer MD5. O banco EXTTXT contém quatro idiomas, IDs, variantes, descritores de 12 bytes, pool de strings Latin-1 e dados auxiliares opacos. A tradução substitui somente as strings do slot inglês e seus offsets. IDs, variantes, controles, substituições formatadas, oito bytes restantes de cada descritor e dados auxiliares são preservados. Os demais idiomas e recursos comprimidos são preservados. O significado completo dos dados auxiliares não foi decodificado; preservá-los não é comprovação de sincronização audiovisual.

O pacote privado inicial altera **422 entradas de Hope Street**, **50 globais** e **90 do frontend**. Há IDs/nomenclaturas sem tradução e textos do restante da campanha ainda não tratados. Não se trata de tradução completa. Acentos foram vistos no menu original no host Linux; quebra de linhas, cenas e sincronização das legendas ainda exigem inspeção.

## Aplicação na ISO do usuário

O APK privado pode conter um dicionário opcional de traduções, com IDs e hashes das revisões, sem arquivos LST/ENT originais. `tools/build_locale_pack.py` recebe um manifesto privado de três arquivos e documentos produzidos por `tools/text_banks.py`. Dados de origem, dicionários e arquivos modificados ficam em `private/` ou `build/`; não pertencem ao Git nem à CI pública.

Ao habilitar **Português: Hope Street e parte dos menus**, o launcher lê os três recursos da ISO validada, confere SHA-256 e integridade do contêiner, aplica a tradução Java fora da UI e grava o cache atomicamente em `files/config/localization/<hash-do-pacote>`. O manifesto é ativado por último. Em erro, o jogo não inicia nessa tentativa: o usuário recebe `UC-LOCALE-001` e pode desativar a opção para usar os textos originais. A preparação pode ser repetida após interrupção.

Antes do boot, o processo do jogo valida nomes, hashes e cache interno e instala substituições imutáveis no leitor de disco. A ISO permanece aberta somente para leitura e não é modificada. Nenhum XBE pode ser substituído. São aceitos apenas os três arquivos conhecidos, existentes na imagem, com revisão exata e limite total de 64 MiB. O pacote atual usa aproximadamente **14 MiB adicionais de RAM** para os contêineres substituídos. Saves ficam separados.

**No jogo, habilitar Options → General → Subtitles/Legendas.** A opção do launcher seleciona os textos; a opção original controla a exibição. O jogo continua sendo responsável por escolher a frase, exibi-la e removê-la. Não há transcrição de áudio, legendas sobrepostas por relógio externo ou tradução automática durante a execução. Vídeos sem legenda nativa identificada continuam sem tradução.

## Validação e limites

- Python e Java produziram bancos traduzidos idênticos para os três arquivos reais, com todos os recursos não textuais comprimidos byte a byte iguais.
- Fixtures autorais verificam variantes, dados auxiliares, controles, hashes, limites, nomes duplicados, contêiner corrompido e recusa a substituir texto fora do último payload.
- O leitor nativo foi testado com ISO autoral: tamanho substituído, leitura concorrente, EOF, recusa de XBE/caminho externo e imagem original inalterada.
- Biblioteca real AArch64 ligada. Isso não comprova execução deste pacote no tablet ou legendas sincronizadas na campanha.

Receita privada:

```sh
python3 tools/build_locale_pack.py private/localization/manifest.json --out private/localization/hope-street-ptbr.bin
python3 scripts/package_private_launcher.py --native-build build/private-runtime --locale-pack private/localization/hope-street-ptbr.bin --out build/private-apk-new
```

O manifesto é uma lista de objetos `name`, `archive` e `translations`. Só os três nomes acima são aceitos. O documento de tradução inclui o hash do banco de origem e é validado antes de gerar o dicionário. Não publicar o manifesto real, fontes comerciais ou arquivos modificados.

## Build privado para o próximo ensaio

Versão **0.4-experimental**, versionCode **4**, build `experimental-05bb54fce75d0b35`, APK de **28.247.992 bytes**. SHA-256: `62d05739c6f20e36c53c46a79939fa7598f1615e062e46ae60508c3df3a34ead`. Assinatura debug comparada com 0.3, alinhamento e bibliotecas empacotadas verificados. A divisão de transporte tem três partes novas, `.001`/`.002`/`.003`, verificadas por concatenação e CRC. Não misturar partes de versões diferentes. [Recibo sanitizado](evidence/runtime-progress/android-native-subtitles.json).

Instalar por cima da versão anterior, preservando dados e a ISO importada. Antes de iniciar, marcar a opção de português se desejar; no jogo, habilitar as legendas em General e ajustar a sensibilidade em Controller. Para a missão, usar Controle Xbox completo. Após entrar na cena lenta, manter a execução por cerca de 30 segundos e exportar o relatório pelo ⋯; o ZIP registra os novos tempos e os resultados da preparação de idioma. Uma captura da cena ajuda a distinguir defeitos de geometria, textura e iluminação. O APK novo ainda não foi executado no tablet.
