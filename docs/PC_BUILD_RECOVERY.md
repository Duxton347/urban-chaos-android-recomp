# Recuperação da compilação PC — execução

**Resultado atual:** núcleo do SDK compilado e memória real testada no Linux. A compilação completa de Urban Chaos ainda depende de entradas do título; há uma tentativa reproduzível do build original em Windows, sem stubs ou status de qualificação fabricado.

## O que foi buscado e recuperado

A busca foi além do README atual: todas as 21 revisões presentes no histórico público, os pacotes-fonte 0.01, 0.03 e 0.18 e o pacote Windows 0.18 foram inventariados. Os quatro ZIPs conferem com os SHA-256 publicados pelo autor. Dois forks públicos também foram inspecionados por árvore de arquivos. Downloads/executáveis ficam apenas em `private/pc-recovery`, sem cache ou publicação dos bytes.

[Inventário de arquivos/histórico e hashes](evidence/pc-recovery/inventory.json), [forks](evidence/pc-recovery/forks.json).

Foram recuperados da versão 0.01:

- Identidade declarada do candidato: Title ID `534300F7`, versão declarada `3`, XDK `5849`, entrada `00082027`, thunk kernel `0044B260`, 146 imports e 17 seções.
- Tamanho XBE de 5.283.840 bytes e SHA-256 já conhecido `b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac`.
- Configuração antiga, 247 seeds de funções, 23 entradas de tracing e scripts originais de análise, identificação, ABI, reparo de metadados e geração C.
- Pin do toolkit antigo `sp00nznet/xboxrecomp`: `14522228d296a7004adb02d6aec8335eee268cae` (MIT).

A referência de identidade está em [config/urban-chaos-candidate.json](../config/urban-chaos-candidate.json). São metadados públicos recuperados, **não leitura independente do XBE nesta sessão**. O campo `version=3` é preservado como declaração histórica; não foi usado para inventar qualificação do certificado.

As versões antigas usam outro gerador e outro runtime. O `config/game.json` e os seeds antigos não são a configuração TOML qualificada da cadeia XboxXBERecomp da 0.18. Não foram transplantados indiscriminadamente para o novo SDK.

## O que os pacotes realmente contêm

| Entrada inspecionada | Código gerado do título | XBE/ISO original | Observação |
|---|---|---|---|
| Source 0.01 | Ausente | Ausente | Configuração e receita antigas presentes; o próprio BUILDING também declara unidades congeladas/artefatos de prova ausentes. |
| Source 0.03 | Ausente | Ausente | Mudança de runtime para XboxXBERecomp; fontes antigas ainda aparecem, mas não constituem a área qualificada nova. |
| Source 0.18 | Ausente | Ausente | Driver atual parte de `build/xbe-full-integration-20261014`. |
| Windows 0.18 | Executável PC já compilado, não C/C++ de título | Ausente | `UrbanChaosXbeTrial.exe`, PE x64, 27.289.088 bytes. |
| Histórico público e dois forks | Nenhuma unidade gerada encontrada pelos padrões registrados | Nenhuma imagem encontrada | Inspeção de nomes/árvores, não prova sobre arquivos privados não publicados. |

O runtime PC publicado tem SHA-256 `791d1e0bcec71ae69c7a7c2534b40b21c93a58110d2ce303bb866a77e28b611b`. A varredura do executável não encontrou uma cópia **bruta com o hash candidato** embutida. Isso não afirma que uma análise binária completa seria impossível; comprova que não recuperamos o XBE original simplesmente do pacote. O `package-manifest.json` dizer que o pacote está completo não significa que o código-fonte de build esteja completo.

## Tentativas executadas

### Núcleo PC real do SDK

```bash
scripts/build_pc_runtime_core.sh
```

Compila os fontes originais de `runtime/kernel/*.cpp`, `core/log.cpp` e `cpu/dispatch.cpp` do pin Burner em biblioteca estática. O SDK permanece sem modificações. O executável de teste liga os componentes de memória/log necessários e chama os serviços reais para reservar, commitar, consultar e liberar memória virtual e verificar coerência das janelas física/write-combine.

[Build e teste passaram](evidence/pc-recovery/runtime-core.log); [proveniência dos fontes compilados](evidence/pc-recovery/kernel-provenance.json). Isso não liga todas as dependências do runtime nem executa boot/gameplay de Urban Chaos. Não há funções de título falsas.

### Cadeia antiga de análise

Executado o parser original do toolkit antigo contra o caminho esperado para a entrada legítima. Resultado: exit 1, arquivo `Default.xbe` ausente. [Log](evidence/pc-recovery/legacy-parser-attempt.log). Não foi usado um XBE sintético para chamar essa tentativa de análise do jogo.

### Build original da versão 0.18

```bash
python scripts/attempt_pc_build.py
```

No runner Windows 2022, o script baixa o Source ZIP público por URL/checksum fixos e executa seu CMake e o target `UrbanChaosXbeTrial` **sem alterar o builder**. Instala um Python 3.12.8 real no caminho esperado pelo wrapper usando junction, e usa CMake 3.31.6 verificado por checksum. O wrapper original é responsável por adquirir o mutex; não se define um mutex fictício nem se cria `integration-status.json`.

O workflow [PC original source reproduction attempt](../.github/workflows/pc-recovery.yml) conserva o exit code do build e registra o status mesmo quando falha. Não tem cache nem upload de artefatos e não recebe dados comerciais. Seu resultado deve ser lido separadamente dos testes de infraestrutura. O resultado remoto será registrado nesta seção depois da execução.

`--prepare-only` permite conferir/descompactar as fontes públicas no Linux, sem afirmar build. Reutilizar a área preparada exige verificar os arquivos contra o ZIP novamente; alterações não são silenciosamente aceitas como fontes originais.

## O que falta para continuar além desses pontos

A entrada mínima que continua ausente é o XBE Xbox legítimo em armazenamento privado na nuvem. A receita antiga o usa desde o primeiro passo de parsing; a cadeia atual também precisa dele para scanner/geração. Para reproduzir a 0.18 ainda são necessários configuração revisada, unidades limpas, backups e qualificação real — ou reconstruir essas entradas com evidências a partir desse XBE.

Não precisamos de upload pelo chat nem do computador pessoal. Nenhuma ROM de terceiros ou cópia cifrada de Black foi usada. O próximo teste deve apontar para a cópia legítima em volume privado. A partir dela é possível executar a análise recuperada e avaliar quais resultados podem servir à cadeia atual, sem garantir antecipadamente a conclusão de G1.

A licença das adaptações Burner permanece uma questão de publicação/reutilização: não foi usada como desculpa para deixar de inspecionar os pacotes ou executar o builder original. Nenhuma implementação própria da Burner foi adaptada ou republicada por esta recuperação; o teste de núcleo usa código MIT do SDK.
