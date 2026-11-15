# Importação Xbox — Redump, XISO e pasta

Aceitar somente dados da cópia legítima do usuário. Não procurar/downloadar discos comerciais, não pedir upload pelo chat e não publicar imagens, executáveis, arquivos de jogo, saves ou cópias criptografadas. Dados ficam em `private/` ou em volume privado na nuvem; o APK final usará SAF no tablet.

## Caminho implementado e limites

`tools/prepare_game.py` importa pasta extraída ou percorre XDVDFS. Reconhece descritor `MICROSOFT*XBOX*MEDIA` no setor 32 da partição; valida assinatura inicial/final, diretórios, limites dos extents, ciclos e colisões de caminhos. Rejeita escapes, links, truncamento, falta de espaço e destinos existentes. Usa staging e só renomeia após validar `Default.xbe`. A imagem original é aberta somente para leitura.

XISO tem base zero. O offset XGD1 Redump é `0x18300000` bytes (setor `0x30600`), consistente com XboxKit `LibXGD/XGD.cs:11`. Há teste com arquivo esparso sintético contendo uma partição nesse offset. **Isso valida endereçamento do importador; não comprova um dump Redump comercial completo nem a integridade de sua partição de vídeo/filler.** O jogo só precisa dos arquivos da partição; conversão arquivística lossless não é requisito do port.

O parser também sonda `0x02080000` como layout XDVDFS alternativo histórico; isso não declara suporte a jogos Xbox 360/XEX. ISO PS2 é rejeitada explicitamente. ISOs comprimidas, imagens reconstruídas exóticas, ZAR e todos os tamanhos/variantes Redump não foram validados. O importador rejeita a origem se não encontrar um descritor suportado.

## XboxKit auditado

Pin `e8c57ca...`, versão declarada 0.7.0. README e `LibXGD/XGD.cs`, `XDVDFS.cs`, `XboxKit/ExtractRedump.cs`, `ProcessXISO.cs`, `.github/workflows/build.yml` e histórico foram inspecionados. Suporta extração de partições e arquivos, vídeo, filler e recursos arquivísticos. Não produz código recompilado.

Dependências: C#/.NET (CLI net6–net10), `SabreTools.Serialization` 3.1.0 e `GrindCore` 0.7.1 em LibXGD. Os dois `.csproj` declaram `PackageLicenseExpression=MIT`, mas a árvore não inclui texto LICENSE. A API GitHub reporta licença desconhecida; a declaração no código é evidência mais específica, sem substituir revisão de dependências. Nenhum C# foi incorporado ou distribuído e o CLI não foi compilado nesta sessão (.NET não instalado). Não portar .NET para o APK nesta fase.

Quem já dispuser de XboxKit validado pode usar sua CLI, por exemplo `xboxkit -n -x own-disc.iso` para extrair a partição sem sobrescrita, e depois importar o XISO com nossas verificações. Essa rota externa não foi executada sobre um disco real e não substitui validação do resultado. Revisar licença/dependências antes de empacotar XboxKit.

## XBE

```bash
python3 tools/inspect_xbe.py private/Default.xbe
python3 tools/prepare_game.py private/own-disc.iso private/imported --require-candidate
python3 tools/prepare_game.py private/extracted-folder private/imported-folder --require-candidate
```

O inspetor emite somente metadados estruturais: SHA-256, certificado (Title ID, versão, disc number, media/region), seções sem payload, bibliotecas/XDK e ordinais kernel. Não imprime chaves ou nome do título. Não autentica RSA nem verifica direitos de posse. Integridade de estrutura e hash candidato não equivale a compatibilidade Android.

Candidato Burner: tamanho **5.283.840**, SHA-256 `b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac`. Title ID e versão esperados não foram inferidos de Black: aguardam leitura do executável real. O hash exato implicitamente identifica a revisão candidata; outras revisões exigem nova auditoria.

## SAF e testes bloqueados

O módulo diagnóstico histórico já contém SAF, staging privado, progresso, cancelamento/retomada, validação do candidato e ponteiro atômico de instalação. É infraestrutura a revisar/integrar ao futuro APK do jogo, não um port. Não usar `MANAGE_EXTERNAL_STORAGE` do Black. Manter `game`, `saves`, `config`, `cache/shaders`, `logs` distintos.

Sem disco: não foram executados extração comercial, comparação integral contra Redump, versão real, completude de assets, boot, streaming de missão ou saves. Os testes atuais usam somente XBE/XISO sintéticos, incluindo falhas e offset Redump; seus bytes são gerados em `build/`/temporários e não são publicados como assets.
