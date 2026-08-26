transformed data {
  int representable = 40000 * 2;
  int wrapped = 50000 * 50000;
  int folded_back = wrapped %/% 50000;
  print(representable, wrapped, folded_back);
}
