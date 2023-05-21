#include "decision_tree.h"

bool Decision_Tree_Predictor::predict(uint64_t ip, int degree, int prefetch_offset, uint64_t hashed_history, int dram_occupancy) {
    // if (degree < 2) {
    //     if (prefetch_offset < 32) {
    //         if (ip <= 4297120) {
    //             return true;
    //         }
    //         else {
    //             return false;
    //         }
    //     }
    //     else {
    //         if (dram_occupancy < 23) {
    //             return true;
    //         }
    //         else {
    //             return false;
    //         }

    //     }
    // }
    // else {
    //     return false;
    // }
   
    // if (degree <= 1.5) {
	// 	if (prefetch_offset <= 31.0) {
	// 		if (ip<= 4297120) {
	// 			if (ip <= 4284847.5) {
	// 				if (ip <= 4275124.0) {
	// 					return true;
	// 				}
	// 				else {
	// 					return false;
	// 				}
	// 			}
	// 			else {
	// 				if (ip <= 4292430.5) {
	// 					return true;
	// 				}
	// 				else {
	// 					return false;
	// 				}
	// 			}
	// 		}
	// 		else {
	// 			if (ip <= 4316361.0) {
	// 				if (dram_occupancy <= 7.5) {
	// 					return false;
	// 				}
	// 				else {
	// 					return false;
	// 				}
	// 			}
	// 			else {
	// 				if (prefetch_offset <= 6.5) {
	// 					return true;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 		}
	// 	}
	// 	else {
	// 		if (dram_occupancy <= 22.5) {
	// 			if (dram_occupancy <= 14.5) {
	// 				if (ip <= 4296440.5) {
	// 					return true;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 			else {
	// 				if (dram_occupancy < 16) {
	// 					return true;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 		}
	// 		else {
	// 			if (hashed_history <= 7466174) {
	// 				if (ip <= 4292212.5) {
	// 					return false;
	// 				}
	// 				else {
	// 					return false;
	// 				}
	// 			}
	// 			else {
	// 				if (hashed_history <= 7467169) {
	// 					return false;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	// else {
	// 	if (prefetch_offset <= 25) {
	// 		if (ip <= 4292681) {
	// 			if (ip <= 4275124) {
	// 				if (ip <= 4274500) {
	// 					return false;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 			else {
	// 				if (ip <= 4292345) {
	// 					return 0;
	// 				}
	// 				else {
	// 					return 0;
	// 				}
	// 			}
	// 		}
	// 		else {
	// 			if (ip <= 4296593) {
	// 				if (prefetch_offset < 4) {
	// 					return true;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 			else {
	// 				if (ip <= 4316361) {
	// 					return false;
	// 				}
	// 				else {
	// 					return true;
	// 				}
	// 			}
	// 		}
	// 	}
	// 	else {
	// 		if (prefetch_offset <= 31) {
	// 			if (degree < 3) {
	// 				if (ip <= 4297108) {
	// 					return false;
	// 				}
	// 				else {
	// 					return false;
	// 				}
	// 			}
	// 			else {
	// 				return false;
	// 			}
	// 		}
	// 		else {
	// 			if (dram_occupancy < 1) {
	// 				if (hashed_history <= 7464709) {
	// 					return false;
	// 				}
	// 				else {
	// 					return false;
	// 				}
	// 			}
	// 			else {
	// 				return false;
	// 			}
	// 		}
	// 	}
	// }
    if (degree <= 1.5) {
		if (prefetch_offset <= 31.0) {
			if (ip <= 4297120.0) {
				if (ip <= 4284847.5) {
					if (ip <= 4275124.0) {
						if (ip <= 4274500.5) {
							return 0;
						}
						else {
							if (dram_occupancy <= 20.5) {
								if (prefetch_offset <= 1.5) {
									if (hashed_history <= 7465878.5) {
										if (dram_occupancy <= 7.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7469607.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (dram_occupancy <= 19.5) {
										if (dram_occupancy <= 14.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (prefetch_offset <= 2.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
							}
							else {
								return 1;
							}
						}
					}
					else {
						return 0;
					}
				}
				else {
					if (ip <= 4292430.5) {
						if (prefetch_offset <= 1.5) {
							if (hashed_history <= 7430228.0) {
								if (hashed_history <= 7429932.5) {
									if (dram_occupancy <= 7.5) {
										if (hashed_history <= 7426736.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 16.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (dram_occupancy <= 14.5) {
										if (dram_occupancy <= 13.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7429985.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (dram_occupancy <= 5.5) {
									if (hashed_history <= 7454052.0) {
										if (ip <= 4292319.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7463112.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (hashed_history <= 7461328.5) {
										if (hashed_history <= 7449706.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7462779.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
						else {
							if (ip <= 4292345.0) {
								if (prefetch_offset <= 2.5) {
									if (hashed_history <= 7448924.5) {
										if (hashed_history <= 7446599.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7461929.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4292212.5) {
										if (dram_occupancy <= 15.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 23.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (hashed_history <= 7463306.0) {
									if (ip <= 4292404.5) {
										if (prefetch_offset <= 2.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										return 0;
									}
								}
								else {
									if (ip <= 4292394.0) {
										if (prefetch_offset <= 2.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										return 0;
									}
								}
							}
						}
					}
					else {
						if (ip <= 4292677.0) {
							if (ip <= 4292631.5) {
								if (prefetch_offset <= 1.5) {
									if (dram_occupancy <= 13.5) {
										if (hashed_history <= 7456486.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 16.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									return 0;
								}
							}
							else {
								return 0;
							}
						}
						else {
							if (prefetch_offset <= -1.5) {
								return 0;
							}
							else {
								if (prefetch_offset <= 3.5) {
									if (ip <= 4295291.5) {
										if (prefetch_offset <= 1.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (prefetch_offset <= 2.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (prefetch_offset <= 4.5) {
										if (hashed_history <= 7423849.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (ip <= 4296189.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
					}
				}
			}
			else {
				if (ip <= 4297745.0) {
					if (dram_occupancy <= 7.5) {
						if (dram_occupancy <= 5.5) {
							if (dram_occupancy <= 4.5) {
								return 0;
							}
							else {
								if (prefetch_offset <= 1.5) {
									if (hashed_history <= 7441864.5) {
										if (hashed_history <= 7423000.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										return 1;
									}
								}
								else {
									return 0;
								}
							}
						}
						else {
							if (prefetch_offset <= 1.5) {
								if (hashed_history <= 7415660.0) {
									if (dram_occupancy <= 6.5) {
										return 1;
									}
									else {
										return 1;
									}
								}
								else {
									if (dram_occupancy <= 6.5) {
										if (hashed_history <= 7462605.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										return 0;
									}
								}
							}
							else {
								return 0;
							}
						}
					}
					else {
						if (prefetch_offset <= 1.5) {
							if (ip <= 4297721.5) {
								if (hashed_history <= 7423585.0) {
									if (ip <= 4297698.5) {
										if (dram_occupancy <= 13.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 18.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7463731.0) {
										if (hashed_history <= 7455013.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (ip <= 4297705.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								return 0;
							}
						}
						else {
							return 0;
						}
					}
				}
				else {
					if (dram_occupancy <= 23.5) {
						if (prefetch_offset <= 3.5) {
							if (ip <= 4316357.5) {
								if (prefetch_offset <= 1.5) {
									if (hashed_history <= 7415013.5) {
										if (dram_occupancy <= 21.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 12.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									return 0;
								}
							}
							else {
								if (ip <= 4316377.5) {
									if (hashed_history <= 7423401.5) {
										if (hashed_history <= 7423385.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
								else {
									return 0;
								}
							}
						}
						else {
							if (dram_occupancy <= 11.5) {
								if (hashed_history <= 7423377.5) {
									if (dram_occupancy <= 6.5) {
										return 1;
									}
									else {
										if (dram_occupancy <= 8.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (dram_occupancy <= 8.5) {
										if (hashed_history <= 7423380.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 10.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (dram_occupancy <= 14.5) {
									if (dram_occupancy <= 13.5) {
										if (hashed_history <= 7423377.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7423379.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7424937.0) {
										if (hashed_history <= 7423379.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
							}
						}
					}
					else {
						if (prefetch_offset <= 1.5) {
							return 1;
						}
						else {
							if (prefetch_offset <= 7.0) {
								return 0;
							}
							else {
								return 1;
							}
						}
					}
				}
			}
		}
		else {
			if (dram_occupancy <= 22.5) {
				if (dram_occupancy <= 14.5) {
					if (ip <= 4296440.5) {
						if (ip <= 4284847.5) {
							if (ip <= 4276473.0) {
								if (hashed_history <= 7440658.0) {
									if (hashed_history <= 7414713.0) {
										if (hashed_history <= 7411496.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7432437.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (dram_occupancy <= 13.5) {
										if (hashed_history <= 7461245.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7444205.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (hashed_history <= 7420538.0) {
									if (hashed_history <= 7420256.0) {
										if (dram_occupancy <= 1.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
								else {
									if (hashed_history <= 7426603.5) {
										if (ip <= 4277007.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7427076.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
						else {
							if (hashed_history <= 7469569.0) {
								if (dram_occupancy <= 12.5) {
									if (ip <= 4295291.5) {
										if (ip <= 4292372.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (ip <= 4296157.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7413693.5) {
										if (hashed_history <= 7413680.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4296184.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (ip <= 4292231.5) {
									if (hashed_history <= 7469668.5) {
										if (hashed_history <= 7469650.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
								else {
									return 1;
								}
							}
						}
					}
					else {
						if (dram_occupancy <= 0.5) {
							if (ip <= 4297107.5) {
								if (hashed_history <= 7412624.5) {
									return 0;
								}
								else {
									if (hashed_history <= 7415983.0) {
										return 1;
									}
									else {
										if (hashed_history <= 7444832.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (ip <= 4297705.5) {
									if (hashed_history <= 7411415.5) {
										if (hashed_history <= 7407489.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7411947.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7414967.5) {
										if (ip <= 4297753.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7437966.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
						else {
							if (hashed_history <= 7418533.0) {
								if (hashed_history <= 7416135.0) {
									if (ip <= 4297120.0) {
										if (dram_occupancy <= 11.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4297721.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4297712.0) {
										if (hashed_history <= 7417339.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
							}
							else {
								if (hashed_history <= 7464131.0) {
									if (ip <= 4297120.0) {
										if (dram_occupancy <= 7.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7431149.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4297712.0) {
										if (ip <= 4297107.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7466692.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
							}
						}
					}
				}
				else {
					if (dram_occupancy <= 15.5) {
						if (hashed_history <= 7465784.0) {
							if (hashed_history <= 7418063.5) {
								if (hashed_history <= 7411447.0) {
									if (ip <= 4297712.0) {
										if (ip <= 4296355.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (ip <= 4297745.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7414852.0) {
										if (hashed_history <= 7412385.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7415633.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
							else {
								if (hashed_history <= 7462113.5) {
									if (ip <= 4284368.0) {
										if (hashed_history <= 7420256.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7446316.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7463667.0) {
										if (hashed_history <= 7463323.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7464601.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
							}
						}
						else {
							if (hashed_history <= 7469668.5) {
								if (ip <= 4297712.0) {
									if (hashed_history <= 7469569.0) {
										if (ip <= 4294585.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7469649.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									return 1;
								}
							}
							else {
								return 0;
							}
						}
					}
					else {
						if (hashed_history <= 7469605.5) {
							if (hashed_history <= 7461313.5) {
								if (dram_occupancy <= 19.5) {
									if (ip <= 4292212.5) {
										if (hashed_history <= 7448262.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 16.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7438867.5) {
										if (hashed_history <= 7413766.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7440827.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (dram_occupancy <= 20.5) {
									if (ip <= 4292345.0) {
										if (hashed_history <= 7463197.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7463214.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4292659.0) {
										if (hashed_history <= 7469584.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7462992.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
						else {
							if (ip <= 4292340.5) {
								if (ip <= 4283356.0) {
									return 1;
								}
								else {
									if (hashed_history <= 7469657.5) {
										if (hashed_history <= 7469649.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
							}
							else {
								if (ip <= 4292538.5) {
									return 1;
								}
								else {
									if (dram_occupancy <= 17.5) {
										return 1;
									}
									else {
										if (hashed_history <= 7469668.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
					}
				}
			}
			else {
				if (hashed_history <= 7466174.0) {
					if (ip <= 4292212.5) {
						if (ip <= 4284368.0) {
							if (hashed_history <= 7414963.0) {
								if (dram_occupancy <= 25.5) {
									if (dram_occupancy <= 24.5) {
										return 1;
									}
									else {
										if (hashed_history <= 7414259.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7414423.0) {
										if (hashed_history <= 7413905.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										return 1;
									}
								}
							}
							else {
								if (hashed_history <= 7415856.0) {
									if (dram_occupancy <= 25.5) {
										if (hashed_history <= 7415596.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 27.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (hashed_history <= 7427787.0) {
										if (dram_occupancy <= 28.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4275429.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
						}
						else {
							if (dram_occupancy <= 24.5) {
								if (hashed_history <= 7461303.0) {
									if (hashed_history <= 7418270.5) {
										if (hashed_history <= 7413635.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7428354.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (hashed_history <= 7463002.0) {
										if (dram_occupancy <= 23.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7463171.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (dram_occupancy <= 31.5) {
									if (hashed_history <= 7446049.0) {
										if (hashed_history <= 7428354.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7463279.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7446543.5) {
										if (hashed_history <= 7446346.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7463279.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
					}
					else {
						if (hashed_history <= 7448290.0) {
							if (ip <= 4292659.0) {
								if (ip <= 4292404.5) {
									if (ip <= 4292345.0) {
										if (hashed_history <= 7446272.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 1;
									}
								}
								else {
									if (hashed_history <= 7413829.0) {
										if (hashed_history <= 7413764.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7442325.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
							}
							else {
								if (ip <= 4296428.0) {
									if (hashed_history <= 7424254.0) {
										if (hashed_history <= 7407826.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (ip <= 4296160.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (dram_occupancy <= 24.5) {
										if (hashed_history <= 7415019.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 27.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
						}
						else {
							if (hashed_history <= 7462842.0) {
								if (ip <= 4292659.0) {
									if (ip <= 4292377.0) {
										if (dram_occupancy <= 24.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 24.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7455227.5) {
										if (dram_occupancy <= 24.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										return 0;
									}
								}
							}
							else {
								if (hashed_history <= 7462991.0) {
									if (ip <= 4292538.5) {
										if (dram_occupancy <= 26.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4294402.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (ip <= 4297120.0) {
										if (dram_occupancy <= 24.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 23.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
						}
					}
				}
				else {
					if (hashed_history <= 7467169.0) {
						if (dram_occupancy <= 23.5) {
							if (hashed_history <= 7466738.0) {
								if (ip <= 4287112.0) {
									return 0;
								}
								else {
									if (ip <= 4297712.0) {
										return 0;
									}
									else {
										return 0;
									}
								}
							}
							else {
								return 0;
							}
						}
						else {
							if (dram_occupancy <= 31.5) {
								if (dram_occupancy <= 27.5) {
									if (dram_occupancy <= 24.5) {
										if (ip <= 4284589.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 26.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (dram_occupancy <= 28.5) {
										if (ip <= 4297712.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										return 0;
									}
								}
							}
							else {
								if (hashed_history <= 7466740.5) {
									return 0;
								}
								else {
									if (hashed_history <= 7467079.0) {
										return 1;
									}
									else {
										return 0;
									}
								}
							}
						}
					}
					else {
						if (ip <= 4292340.5) {
							if (ip <= 4283356.0) {
								if (dram_occupancy <= 27.5) {
									if (dram_occupancy <= 24.0) {
										return 1;
									}
									else {
										if (dram_occupancy <= 25.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (dram_occupancy <= 28.5) {
										return 1;
									}
									else {
										if (dram_occupancy <= 30.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								return 0;
							}
						}
						else {
							if (hashed_history <= 7467334.5) {
								if (hashed_history <= 7467275.0) {
									return 1;
								}
								else {
									return 0;
								}
							}
							else {
								if (hashed_history <= 7469649.5) {
									if (ip <= 4292538.5) {
										return 1;
									}
									else {
										if (hashed_history <= 7469579.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (dram_occupancy <= 24.5) {
										if (hashed_history <= 7469668.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										return 1;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else {
		if (prefetch_offset <= 25.0) {
			if (ip <= 4292681.0) {
				if (ip <= 4275124.0) {
					if (ip <= 4274500.5) {
						return 0;
					}
					else {
						if (hashed_history <= 7469574.5) {
							if (dram_occupancy <= 30.5) {
								if (degree <= 3.5) {
									if (dram_occupancy <= 29.5) {
										if (hashed_history <= 7434569.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7448483.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (dram_occupancy <= 9.5) {
										if (hashed_history <= 7432342.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 29.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (hashed_history <= 7438436.0) {
									return 1;
								}
								else {
									if (hashed_history <= 7444235.0) {
										return 0;
									}
									else {
										return 1;
									}
								}
							}
						}
						else {
							if (prefetch_offset <= 1.5) {
								if (dram_occupancy <= 24.0) {
									if (hashed_history <= 7469616.5) {
										if (degree <= 3.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (degree <= 2.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (degree <= 2.5) {
										return 1;
									}
									else {
										return 0;
									}
								}
							}
							else {
								if (dram_occupancy <= 20.5) {
									if (dram_occupancy <= 19.5) {
										if (dram_occupancy <= 18.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
								else {
									return 1;
								}
							}
						}
					}
				}
				else {
					if (ip <= 4292345.0) {
						if (ip <= 4292212.5) {
							if (degree <= 2.5) {
								if (ip <= 4284847.5) {
									return 0;
								}
								else {
									if (prefetch_offset <= 1.5) {
										if (dram_occupancy <= 7.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 15.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
							else {
								if (prefetch_offset <= 3.5) {
									if (prefetch_offset <= 2.5) {
										return 0;
									}
									else {
										if (ip <= 4277007.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (degree <= 3.5) {
										if (hashed_history <= 7417686.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										return 0;
									}
								}
							}
						}
						else {
							if (ip <= 4292319.5) {
								if (ip <= 4292259.5) {
									if (dram_occupancy <= 31.5) {
										if (ip <= 4292221.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (degree <= 3.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (degree <= 2.5) {
										if (prefetch_offset <= 1.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										return 0;
									}
								}
							}
							else {
								if (hashed_history <= 7446162.0) {
									if (hashed_history <= 7430190.5) {
										if (dram_occupancy <= 26.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 21.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7446226.0) {
										if (dram_occupancy <= 14.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (prefetch_offset <= 1.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
					}
					else {
						if (ip <= 4292400.5) {
							if (degree <= 2.5) {
								if (prefetch_offset <= 1.5) {
									if (dram_occupancy <= 5.5) {
										if (ip <= 4292372.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 8.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									return 0;
								}
							}
							else {
								return 0;
							}
						}
						else {
							if (ip <= 4292495.5) {
								return 0;
							}
							else {
								if (ip <= 4292529.5) {
									return 1;
								}
								else {
									return 0;
								}
							}
						}
					}
				}
			}
			else {
				if (ip <= 4296593.0) {
					if (prefetch_offset <= 3.5) {
						if (prefetch_offset <= -1.5) {
							return 0;
						}
						else {
							if (ip <= 4295291.5) {
								if (prefetch_offset <= 1.5) {
									if (degree <= 3.5) {
										if (hashed_history <= 7463670.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7414804.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4294391.5) {
										if (hashed_history <= 7415235.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7414811.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (prefetch_offset <= 1.5) {
									if (degree <= 3.5) {
										if (degree <= 2.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7464019.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7419295.0) {
										if (dram_occupancy <= 31.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (degree <= 2.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
					}
					else {
						if (degree <= 2.5) {
							if (prefetch_offset <= 4.5) {
								if (hashed_history <= 7415059.5) {
									if (ip <= 4294415.5) {
										if (hashed_history <= 7414800.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (ip <= 4294429.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (hashed_history <= 7463669.0) {
										if (hashed_history <= 7463661.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 6.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (prefetch_offset <= 12.5) {
									if (hashed_history <= 7414983.5) {
										if (hashed_history <= 7414880.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7414986.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4294382.0) {
										if (ip <= 4294364.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7414933.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
						}
						else {
							if (prefetch_offset <= 12.5) {
								if (dram_occupancy <= 31.5) {
									if (prefetch_offset <= 4.5) {
										if (hashed_history <= 7415107.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7414795.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (ip <= 4296264.5) {
										if (ip <= 4296165.0) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4296428.0) {
											return 0;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (degree <= 3.5) {
									if (hashed_history <= 7423737.0) {
										if (ip <= 4294356.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (hashed_history <= 7423754.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									return 0;
								}
							}
						}
					}
				}
				else {
					if (ip <= 4316361.0) {
						return 0;
					}
					else {
						if (prefetch_offset <= 6.5) {
							if (ip <= 4316377.5) {
								if (hashed_history <= 7423377.5) {
									if (dram_occupancy <= 1.0) {
										if (degree <= 3.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 7.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (degree <= 3.5) {
										if (dram_occupancy <= 8.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
									else {
										if (hashed_history <= 7423379.0) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								return 0;
							}
						}
						else {
							if (hashed_history <= 7423377.5) {
								if (dram_occupancy <= 12.5) {
									if (dram_occupancy <= 8.5) {
										if (dram_occupancy <= 7.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (degree <= 3.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
								else {
									if (dram_occupancy <= 17.5) {
										if (dram_occupancy <= 13.5) {
											return 1;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 20.5) {
											return 1;
										}
										else {
											return 1;
										}
									}
								}
							}
							else {
								if (hashed_history <= 7423382.5) {
									if (dram_occupancy <= 6.5) {
										if (dram_occupancy <= 3.5) {
											return 0;
										}
										else {
											return 1;
										}
									}
									else {
										if (dram_occupancy <= 8.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (dram_occupancy <= 8.5) {
										if (dram_occupancy <= 5.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (dram_occupancy <= 11.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else {
			if (prefetch_offset <= 31.0) {
				if (degree <= 2.5) {
					if (ip <= 4297107.5) {
						if (ip <= 4286815.5) {
							return 0;
						}
						else {
							if (hashed_history <= 7464775.0) {
								if (hashed_history <= 7417305.0) {
									if (ip <= 4296160.5) {
										if (hashed_history <= 7417086.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4296189.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (hashed_history <= 7441381.5) {
										if (hashed_history <= 7418892.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
									else {
										if (ip <= 4296189.5) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
							else {
								return 0;
							}
						}
					}
					else {
						return 0;
					}
				}
				else {
					return 0;
				}
			}
			else {
				if (dram_occupancy <= 0.5) {
					if (hashed_history <= 7464709.0) {
						if (hashed_history <= 7417547.0) {
							if (hashed_history <= 7417531.5) {
								if (ip <= 4285919.5) {
									if (hashed_history <= 7413311.5) {
										return 0;
									}
									else {
										if (hashed_history <= 7416122.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
								else {
									if (hashed_history <= 7416891.5) {
										return 0;
									}
									else {
										if (hashed_history <= 7416963.0) {
											return 0;
										}
										else {
											return 0;
										}
									}
								}
							}
							else {
								if (ip <= 4297705.5) {
									return 0;
								}
								else {
									return 0;
								}
							}
						}
						else {
							return 0;
						}
					}
					else {
						if (ip <= 4297705.5) {
							return 0;
						}
						else {
							if (hashed_history <= 7464786.5) {
								return 0;
							}
							else {
								return 0;
							}
						}
					}
				}
				else {
					return 0;
				}
			}
		}
	}
}

void Decision_Tree_Predictor::update_result(bool result) {
    if (result) {
        positive_count++;
    }
    total_count++;
}