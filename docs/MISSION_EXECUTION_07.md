# Missão e touch — investigação 0.7

## Novo relato do tablet

`tincon.zip` confirma 0.6 (`experimental-bef2a30e191f2ccc`): primeiro evento de visibilidade aos 312,577 s e cerca de 1,154 apresentações/s no intervalo final. Continua injogável segundo o proprietário. Os tempos não são um benchmark pareado com 0.5. A versão 0.6 reduziu submissões/esperas, mas **não resolveu o gargalo principal nem os defeitos gráficos**.

Snapshot final: 5.920 submissões, 136.252 polls de visibilidade, 191.491 µs acumulados de IO, nenhuma leitura em andamento. Cerca de 15,5 s em verificações de espera, 17,7 s nos draws e 19,5 s de readback; operações aninhadas não podem ser somadas. Compilação de módulos custa cerca de 0,5 s. As amostras guest apontam repetidamente para rotinas de alocação, mas não são stacks desenroladas. [Resumo sanitizado](evidence/runtime-progress/tablet-mission-06.json).

O registro Java confirma modo **menu** e cache experimental solicitado. O modo menu não apresentava gatilhos nem câmera. Isso explica controles ausentes, sem explicar a velocidade da missão.

## Retornos que mudam de slot sem mudar de destino

Uma sonda privada Linux interceptou `__cxa_throw`, sem alterar instruções guest. Duas rotinas de prólogo/epílogo do executável original dominam as exceções: preservam o PC de retorno, mas mudam o slot onde ele fica na pilha. O contrato anterior tratava toda mudança de slot como transferência não local. Esse custo ocorre mesmo durante operações comuns de alocação.

A otimização é restrita às duas funções cujo corpo gerado tem hash revisado em [configuração](../config/research/relocated-return-helpers.json). Mantém todas as instruções originais e a gravação dos flags/metadados. Só dispensa o desenrolamento C++ quando o destino é exatamente o retorno esperado. Destino diferente ainda transfere; entrada por continuação ainda entrega o retorno ao dispatcher; as demais funções e a proteção de callbacks por slot permanecem intactas. Não se afrouxou globalmente a verificação de troca de pilha.

`tools/relocate_return_helpers.py` recusa fonte/corpo divergente, função duplicada ou guard diferente. `scripts/build_return_helper_archive.py` recompila somente a unidade afetada, substitui seu único membro numa cópia do arquivo estático, conserva o baseline e verifica a lista de membros. O recibo identifica o baseline, fonte alterada e resultado. Arquivo original, gerado, objeto, arquivo estático e APK ficam fora do Git. Isso é pós-processamento explícito da geração revisada, não qualificação automática de outro XBE.

## Controles

O padrão e a migração única passam a ser **Controle Xbox completo**, preservando os layouts salvos de cada perfil. A seleção manual posterior de menu continua disponível. Todos os botões Xbox ficam acessíveis, incluindo LT, RT, Y, Black/White, D-pad e L3/R3, além de A/B/X/START/BACK. Seus nomes identificam fontes reais de `XInputGetState`; o esquema escolhido dentro do jogo determina as ações. A inspeção do banco original confirma ações como tiro, tiro secundário, escudo, recarga, ordens, agachar e zoom; ainda não é uma validação de cada associação no esquema de controle em uso. Não inventamos vínculos de equipe/escudo.

A mira por arrasto não exige joystick direito: um dedo na área livre do lado esquerdo envia pulsos para o analógico direito; botões e joystick de movimento têm prioridade. Em ⋯ → Mira pode-se escolher esquerda, direita ou o joystick antigo. Ao parar de arrastar, o eixo zera após 90 ms; soltar, perder foco, editar ou cancelar zera imediatamente. Movimento, mira e botões mantêm proprietários independentes por dedo. A sensibilidade e inversão originais continuam no menu do jogo. A conversão usa o analógico Xbox, não uma câmera nativa com deslocamento angular direto; a resposta visual continua limitada pelo FPS guest.

## Testes e limites

Passaram 58 testes Python, incluindo seleção estrita dos helpers e execução C++ autoral do retorno realocado, destino divergente e entrada de continuação. As verificações de HUD, fontes Xbox e diagnóstico passaram, incluindo propriedade do dedo, direção, saturação, parada e cancelamento da mira. A primeira tentativa de Gradle omitiu parâmetros privados obrigatórios; as falhas foram registradas, e a compilação com os parâmetros corretos passou.

A sonda de exceções é instrumentação, não medição de FPS nem teste de gameplay. Execução simultânea com builds também impede usar seus tempos como benchmark. Vulkan de software Linux não substitui Adreno. Carregamento completo, imagem correta, missão jogável e desempenho Android continuam dependentes de evidência; não são declarados corrigidos pelo HUD ou pela redução de exceções.

## Resultado desta sessão

Sonda inicial de 10 s: 105.816 exceções no baseline, 103.789 originadas nos dois helpers. Na sonda alterada, 2.174 exceções totais e nenhuma nesses helpers; outras transferências continuam reais. Isso confirma a remoção do mecanismo desnecessário, **não equivale a esse percentual de ganho de FPS**. As sondas não são pareadas por workload e houve carga concorrente. O rebuild pelo script público produziu exatamente o mesmo SHA-256 do arquivo estático Linux usado no teste. [Recibo](evidence/runtime-progress/helper-return-07.json).

O ensaio original Linux de 180,039 s apresentou o frame 1.800 e a abertura 3D foi inspecionada na captura privada. Nenhum pedido de recurso ausente ou exceção fatal foi registrado. A execução acabou pelo limite programado/SIGTERM, não por concluir missão. Compilações e uma sonda concorrentes impedem comparar seus tempos com versões anteriores. **Missão interativa e fluidez não foram comprovadas.**

APK privado 0.7, `experimental-c6d3b223cf275f48`, versionCode 7, 28.252.088 bytes, SHA-256 `255e0be233311b5e5ccc5a0497301959591be55eafbe665e687664569222c385`. Bibliotecas AArch64 ligadas, Gradle e assinatura aprovados; certificado igual ao 0.6. Três partes de transporte verificadas por concatenação, CRC e hash do APK interno. [Recibo](evidence/runtime-progress/android-helper-07.json). Não contém recursos da ISO e não foi publicado no GitHub.

Instalação por cima preserva os dados. Arrasto fica à esquerda por padrão, com seleção de lado no ⋯. Manter cache experimental desativado durante comparação; português pode permanecer ativo. Esta entrega é uma correção parcial: não se declara missão jogável nem todos os botões associados semanticamente ao esquema escolhido. Próxima investigação deve medir o custo dos demais retornos/continuations e do processamento guest durante gameplay, além da fidelidade gráfica, sem voltar a atribuir toda a demora à GPU ou ao armazenamento.
