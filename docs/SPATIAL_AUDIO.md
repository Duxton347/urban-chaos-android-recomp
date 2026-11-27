# Áudio espacial: integração e limites

Após corrigir a retenção de superfícies, o título passou pelo bloqueio anterior e falhou em `0x004253A3`, por acessar o layout Xbox de um objeto de áudio do host. O consumidor original `0x00251A80` inicializa cone, orientação, volume externo e fatores espaciais. Foram conferidos os argumentos e os retornos das seis interfaces de buffer antes de ligá-las ao mixer.

| API | Binding desta variante | ABI |
|---|---|---|
| SetConeAngles | `00425F5B` | quatro argumentos na pilha |
| SetConeOrientation | `00425F7F` | cinco argumentos na pilha |
| SetConeOutsideVolume | `00425FB4` | três argumentos na pilha |
| SetDistanceFactor | `004260A6` | três argumentos na pilha |
| SetRolloffFactor | `004260CA` | três argumentos na pilha |
| SetDopplerFactor | `004260EE` | três argumentos na pilha |

A primeira execução com cone passou aquelas três chamadas e revelou os fatores seguintes (`0x004255A8`). Os campos internos +44/+48/+4C e o consumidor foram comparados à interface X_DS3DBUFFER registrada nas referências do projeto. Não se distribui implementação de terceiros dessa referência.

`directional-audio.patch` usa os helpers autorais `native/audio/cone.h` e `spatial.h`. O cone usa ângulos completos em graus e volume em centésimos de dB; o ganho direcional é aplicado ao mixer. Rolloff altera a atenuação inversa, e velocidades/fatores Doppler alteram o avanço real das amostras. DistanceFactor converte unidades para o cálculo Doppler: não escala indevidamente a razão entre distância e MinDistance. Mudanças adiadas são separadas por propriedade e aplicadas em CommitDeferredSettings. Inputs inválidos não substituem o último estado válido.

GetVoiceProperties continua com 92 bytes: contribuição de distância em +48, cone em +4C, pitch Doppler em +50. As contribuições I3DL2 ainda não implementadas permanecem declaradas como zero; não se reporta DSP inexistente.

## Validação e limites

Fixtures públicos autorais verificam direção frontal/traseira, transição angular, mute, parâmetros inválidos, atualização imediata/adiada independente, rolloff, conversão de unidades e aproximação/afastamento Doppler. Os contratos também compilaram AArch64. No host limpo, o probe HLE verifica os efeitos nos campos reais da voz, inclusive commit, rejeições e pilha, além dos testes anteriores de pitch, streams e lifetime.

O mixer de pesquisa continua uma implementação estéreo de diagnóstico. O cálculo usa velocidade do som de 343,5 m/s e proteção numérica próxima da singularidade sônica; fidelidade supersônica não foi qualificada. Curvas customizadas mantêm o caminho anterior do SDK; sua combinação com fatores exige comparação adicional. HRTF, efeitos I3DL2, precisão espacial completa e sincronismo audível ainda não foram aprovados. Posição, distâncias e outras APIs preexistentes também precisam de auditoria completa de atualização adiada.

Amostras finitas e não nulas não provam que música, vozes e efeitos estão corretos. O sink atual descarta o som. A saída SDL3 e a validação audível por cena permanecem gates separados. [Contratos e ensaios reproduzíveis](RUNTIME_CONTRACTS.md).
