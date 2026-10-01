# Evolution monitor (generation 5 done, 32.2 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c33 | 8.94 | 6 | 6.7 | 94 | learning_rate 0.04->0.06267, plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.73385, istdp_rate 50.0->74.10519, istdp_target 0.02->0.02421, consolidation_rate 0.0->0.02862, consolidated_budget 0.5->0.86684, assembly_inhibition 0.0->0.00063, mode_tau 30.0->25.67333, fatigue_gain3 0.0638->0.07178, fire_threshold3 0.0256->0.03678, winners3 3->4, channel_winners3 4->3 |
| c29 | 8.87 | 6 | 6.7 | 94 | plastic_budget 4.0->4.16408, istdp_target 0.02->0.02722, fire_threshold3 0.0256->0.02666, winners3 3->4 |
| c11 | 8.79 | 12 | 6.5 | 97 | plastic_budget 4.0->3.9007, assembly_inhibition 0.0->0.00063 |
| c3 | 8.68 | 4 | 6.5 | 94 | learning_rate 0.04->0.0391, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917 |
| c27 | 8.67 | 2 | 6.5 | 88 | learning_rate 0.04->0.0391, covariance 1.0->0.89106, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917, channel_winners3 4->3 |
| c16 | 8.66 | 2 | 6.5 | 94 | plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.86922, istdp_rate 50.0->74.10519, consolidation_rate 0.0->0.00569, assembly_inhibition 0.0->0.00063, fire_threshold3 0.0256->0.03678 |
| prime | 8.65 | 12 | 6.4 | 95 | (Prime) |
| c32 | 8.65 | 2 | 6.5 | 88 | learning_rate 0.04->0.04686, plastic_budget 4.0->4.34164, fatigue_gain3 0.0638->0.10403 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 0 | 18 |
| capacity | 7 | 17 |
| efficiency | 0 | 16 |
| wordpairs | 0 | 15 |
| continual | 5 | 17 |
| retention | 5 | 12 |
| order | 0 | 15 |

- A 40% cue brings back on average 76% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): z>q: 2, q>a: 1, w>k: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 5, smile>river: 2, smile>house: 1, cloud>mouse: 1, light>house: 1

## Picture so far (all generations together)

- Brain tests so far: 138 (52 versions, generation 6).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 9 | 138 | 0 | 12 |
| capacity | 31 | 137 | 1 | 12 |
| efficiency | 9 | 136 | 2 | 12 |
| wordpairs | 2 | 135 | 0 | 12 |
| continual | 39 | 137 | 2 | 12 |
| retention | 55 | 132 | 2 | 12 |
| order | 1 | 135 | 0 | 12 |

- Old memories getting worse (all tests): wiped out -0.011, pushed aside by new memories +0.038 on average; of 27 failed 'keep old memories' tests, 24 were mainly pushed aside (interference), 3 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 642, stored but NOT recalled 54, not stored 0.
- Big-load probe (true word recall %, mean over brains): c11 32 words 74% (n=6); c11 64 words 36% (n=6); c21 32 words 66% (n=2); c21 64 words 39% (n=2); c26 32 words 69% (n=2); c26 64 words 34% (n=2); c29 32 words 72% (n=2); c29 64 words 38% (n=2); c3 32 words 81% (n=2); c3 64 words 36% (n=2); c33 32 words 69% (n=2); c33 64 words 41% (n=2); c9 32 words 69% (n=2); c9 64 words 22% (n=2); prime 32 words 70% (n=6); prime 64 words 36% (n=6)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| winners3 | +0.28 | +0.22 | +0.23 |
| encoding_suppression | +0.25 | +0.13 | +0.24 |
| mode_tau | +0.22 | +0.20 | +0.13 |
| plastic_budget | -0.21 | -0.17 | -0.11 |
| channel_winners3 | +0.16 | +0.17 | +0.10 |
| inhibition_radius3 | -0.15 | -0.13 | -0.05 |
| hetero_ltd | +0.14 | +0.13 | +0.05 |
| istdp_target | +0.13 | +0.01 | +0.14 |
| covariance | +0.12 | +0.04 | +0.11 |
| line_recency | +0.09 | +0.14 | +0.03 |
| fatigue_gain3 | +0.09 | +0.09 | +0.08 |
| learning_rate | +0.08 | -0.04 | +0.11 |
| soft_bound | +0.08 | +0.03 | +0.07 |
| order_tau | +0.08 | +0.02 | +0.14 |
| consolidation_rate | -0.07 | -0.15 | -0.05 |
| consolidated_budget | +0.07 | +0.09 | +0.12 |
| presynaptic_bound | -0.07 | +0.01 | -0.10 |
| order_gain | +0.06 | -0.01 | +0.12 |
| assembly_inhibition | -0.05 | -0.12 | +0.05 |
| fire_threshold3 | -0.03 | -0.00 | -0.03 |
| istdp_rate | +0.02 | +0.11 | +0.05 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.