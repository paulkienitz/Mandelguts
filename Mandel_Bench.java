import java.io.*;

// Compile by going "javac *.java", then run with "java -cp . Mandel_Bench".

public class Mandel_Bench
{
	public static int CalculateRange(int limit, int loopmax, double l, double t, double r, double b, int w, int h)
	{
	    int x, y, pc = 0;
	    int z = 0;
	    PointCalculator.Complex[] loopcheck = new PointCalculator.Complex[loopmax];
	    for (y = 0; y < h; y++)
	    {
	        for (x = 0; x < w; x++)
	        {
	            PointCalculator.Complex p0 = new PointCalculator.Complex(l + (x + 0.5) * (r - l) / w, t + (y + 0.5) * (b - t) / h);
	            PointCalculator.Complex p = new PointCalculator.Complex(p0);
	            z ^= PointCalculator.DoMandelPoint(p0, p, limit, loopmax, true, loopcheck);
	        }
	        System.out.print(y);
	        System.out.print("\r");
	        System.out.flush();
	    }
	    return z;
	}


	public static void main(String[] args)
	{
	    double l = -2.75, r = 1.75;
	    int w = 1920, h = 1080;
	    double t = (r - l) * h / (2.0 * w);
	    double b = -t;
	    CalculateRange(50000, 500, l, t, r, b, w, h);
	    System.out.print("Done.  Total loop-check count is ");
	    System.out.print(PointCalculator.total_loopchecks);
	    System.out.println(".");
	}
}
