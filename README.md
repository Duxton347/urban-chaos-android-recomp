# Urban Chaos Android — infraestrutura e diagnóstico

O projeto entrega ferramentas públicas de auditoria/recompilação e um **APK de diagnóstico independente**. Urban Chaos não foi recompilado nem executado: faltam o XBE Xbox e a geração reproduzível (G1). A ISO PS2 não substitui o executável Xbox.

- [Auditoria e evidências](docs/AUDIT.md)
- [Entradas ausentes](docs/MISSING_INPUTS.md)
- [Compatibilidade e diferenças dos runtimes](docs/COMPATIBILITY_MATRIX.md)
- [Gates e status](docs/PLAN.md)
- [Instalação e testes manuais no tablet](docs/DEVICE_TESTS.md)
- [Entrega e hash do APK](docs/DELIVERY.md)

## Build Linux

Use Linux x86_64, Python 3.11+ com `tarfile` que suporte `filter='data'`, Git, GCC/G++ com C++20, Make e acesso HTTPS aos fornecedores públicos. Reserve aproximadamente 6–8 GiB livres para dependências e builds. Não depende do computador pessoal nem de ADB.

```bash
cd /workspace/UrbanChaos-Android
scripts/setup_linux.sh
scripts/test.sh
scripts/build_android.sh assembleDebug lintDebug
JAVA_HOME="$PWD/.toolchain/jdk/jdk-17.0.17+10" python3 scripts/verify_apk.py
```

`setup_linux.sh` baixa fontes nos commits e arquivos nos checksums de `dependencies.lock.json`, instala ferramentas localmente em `.toolchain/` e compila apenas scanner/recompilador host. O SDK manager apresenta/aceita os termos das dependências necessárias à instalação. Não use o CMake raiz da Burner para compilar no Linux/Android.

Versões: Java 17.0.17+10, Gradle 8.7, AGP 8.5.2, SDK 34 (platform rev.3, build-tools 34.0.0), NDK 28.2.13676358, CMake 3.31.6, SDL release-3.4.16 (`fa2c02bb6e21974a89ea9824bc53c9932abe5f9c`). Somente `arm64-v8a`; mínimo API 29. O SDK manager pode oferecer revisões novas sob o mesmo identificador: compare os `source.properties` ao lockfile e interrompa em caso de drift, sem atualizar pins implicitamente. Os hashes publicados dos arquivos Google e checksums de metadados locais estão no lockfile; dependências Maven são verificadas por `android/gradle/verification-metadata.xml`.

O script Android respeita o proxy fornecido e usa o truststore Java do Linux gerenciado quando o proxy está presente. Em outro ambiente, ajuste a configuração de proxy/truststore à política local, mantendo TLS verificado.

O APK aparece em `android/app/build/outputs/apk/debug/app-debug.apk`. Uma chave debug nova é criada em `.toolchain/debug.keystore`, fora do Git. Preserve-a para atualizar esta instalação sem perder dados. Alterar assinatura exige outra instalação e não é uma atualização compatível. Nenhum APK release é assinado automaticamente; a chave release pertence ao proprietário.

## Importação e inspeção Linux

Manter conteúdo comercial em `private/`, já ignorado pelo Git:

```bash
python3 tools/inspect_xbe.py private/Default.xbe
python3 tools/prepare_game.py private/xbox-disc.iso private/imported --require-candidate
# Alternativa: pasta Xbox extraída, contendo Default.xbe na raiz.
python3 tools/prepare_game.py private/xbox-folder private/imported-folder --require-candidate
```

O inspetor mostra SHA-256, estrutura, certificado sem nome do título/chaves, seções, bibliotecas e ordinais kernel. Não emite conteúdo das seções. O importador Linux valida o grafo XDVDFS antes de extrair, rejeita escapes/links/colisões/truncamento, verifica espaço e usa staging com renomeação final; não substitui destinos existentes. Suporta XISO com partição em zero e offsets Xbox conhecidos `0x30600`/`0x4100` setores. Imagens comprimidas, formatos de emulador e outros layouts não são suportados.

A referência inicial do XBE é **5.283.840 bytes**, SHA-256 `b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac`. Identificar essa referência não prova compatibilidade Android.

## Scanner e recompilador

Após obter o XBE legítimo e revisar uma configuração explícita:

```bash
python3 tools/recompile_xbe.py private/Default.xbe \
  --config private/config/urban-chaos.toml --out build/title-attempt-001
```

O wrapper exige runtime limpo no pin e receita/hash dos binários host. Executa scanner, conserva a configuração escaneada para análise e usa a configuração revisada fornecida para duas gerações; compara os hashes e registra logs/recibo. O diretório de saída deve ser novo e ficar em `build/`, ignorado pelo Git. Mesmo uma geração determinística permanece `REQUIRES_REVERSE_ENGINEERING`, sem qualificação automática de G1. Revisar símbolos ausentes, limites, ABI, hooks e callbacks com evidência real antes de integrar ARM64. Não foi executado sobre Urban Chaos nesta sessão.

## Android: armazenamento, importação e HUD

O app usa SAF para arquivo/pasta, permissão de URI persistente, armazenamento privado e staging com cancelamento/retomada. Nunca solicita `MANAGE_EXTERNAL_STORAGE`. O SHA-256 candidato é exigido antes de ativar. `game/active.json` aponta para uma instalação concluída; instalações anteriores e saves não são removidos pelo importador.

Dados separados: `files/game`, `files/saves`, `files/config`, `files/cache/shaders`, `files/logs`. Há journal/staging privado para importações. Desinstalar o app remove esses dados. O diagnóstico não cria saves de jogo nem shader cache de jogo. O HUD salva coordenadas normalizadas em `config/hud.json`; permite arrastar, testar multitouch e observar gamepad. Seus identificadores não têm vínculo com Urban Chaos.

O teste Vulkan apresenta cores sintéticas via SDL3/swapchain. O relatório exportado registra o modelo/Android e logs da GPU/driver quando disponíveis. Instalação, lifecycle e apresentação em hardware ainda exigem o [roteiro manual](docs/DEVICE_TESTS.md).

## Direitos e limites

Não distribuir discos, XBE, código gerado do título, fixtures comerciais ou saves. Burner e Black foram inspecionados como referências; nenhuma adaptação própria desses projetos foi incorporada. A licença geral das adaptações Burner continua sem esclarecimento. Os textos e avisos dos componentes reutilizados estão em `licenses/` e no APK. O diagnóstico é uma implementação independente; não representa uma versão jogável, menu funcional ou campanha verificada.
