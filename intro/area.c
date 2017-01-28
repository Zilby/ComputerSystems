int square_area(int xx) {
  return xx * xx;
}

int main(int argc, char* argv[]) {
  int side = 4;
  int area = square_area(side);
  printf("Area is: %d\n", area);
  return 0;
}
