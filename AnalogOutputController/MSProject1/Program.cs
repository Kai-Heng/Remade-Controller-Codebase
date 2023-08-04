using NationalInstruments.DAQmx;
using NationalInstruments.Examples.ContGenVoltageWfm_IntClk;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSProject1
{
    public enum WaveformType
    {
        SineWave = 0,
        SquareWave = 1
    }

    public class Program
    {

        public Program(Timing timingSubobject,
            string desiredFrequency,
            string samplesPerBuffer,
            string cyclesPerBuffer,
            string type,
            string amplitude)
        {
            WaveformType t = new WaveformType();
            t = WaveformType.SquareWave;
            if (type == "Square Wave")
                t = WaveformType.SquareWave;
            else if(type == "Sine Wave")
            {
                t = WaveformType.SineWave;
            }
            else
                Debug.Assert(false, "Invalid Waveform Type");

            Init(
                timingSubobject,
                Double.Parse(desiredFrequency),
                Double.Parse(samplesPerBuffer),
                Double.Parse(cyclesPerBuffer),
                t,
                Double.Parse(amplitude));
        }

        private void Init(
            Timing timingSubobject,
            double desiredFrequency,
            double samplesPerBuffer,
            double cyclesPerBuffer,
            WaveformType type,
            double amplitude)
        {
            if (desiredFrequency <= 0)
                throw new ArgumentOutOfRangeException("desiredFrequency", desiredFrequency, "This parameter must be a positive number");
            if (samplesPerBuffer <= 0)
                throw new ArgumentOutOfRangeException("samplesPerBuffer", samplesPerBuffer, "This parameter must be a positive number");
            if (cyclesPerBuffer <= 0)
                throw new ArgumentOutOfRangeException("cyclesPerBuffer", cyclesPerBuffer, "This parameter must be a positive number");

            // First configure the Task timing parameters
            if (timingSubobject.SampleTimingType == SampleTimingType.OnDemand)
                timingSubobject.SampleTimingType = SampleTimingType.SampleClock;

            _desiredSampleClockRate = (desiredFrequency * samplesPerBuffer) / cyclesPerBuffer;
            _samplesPerCycle = samplesPerBuffer / cyclesPerBuffer;

            // Determine the actual sample clock rate
            timingSubobject.SampleClockRate = _desiredSampleClockRate;
            _resultingSampleClockRate = timingSubobject.SampleClockRate;

            _resultingFrequency = _resultingSampleClockRate / (samplesPerBuffer / cyclesPerBuffer);

            switch (type)
            {
                case WaveformType.SineWave:
                    _data = GenerateSineWave(_resultingFrequency, amplitude, _resultingSampleClockRate, samplesPerBuffer);
                    break;

                case WaveformType.SquareWave:
                    _data = GenerateSquareWave(_resultingFrequency, amplitude, _resultingSampleClockRate, samplesPerBuffer);
                    break;
                default:
                    // Invalid type value
                    Debug.Assert(false);
                    break;
            }
        }

        public double[] Data
        {
            get
            {
                return _data;
            }
        }

        public double ResultingSampleClockRate
        {
            get
            {
                return _resultingSampleClockRate;
            }
        }

        public static double[] GenerateSquareWave(double frequency, double amplitude, double sampleClockRate, double samplesPerBuffer)
        {
            double deltaT = 1 / sampleClockRate;
            int intSamplesPerBuffer = (int)samplesPerBuffer;

            double[] rVal = new double[intSamplesPerBuffer];

            for(int i = 0; i<intSamplesPerBuffer; i++)
            {
                double t = i * deltaT;
                rVal[i] = amplitude * Math.Sin((2.0 * Math.PI) * frequency * t) + amplitude * Math.Sin((6.0 * Math.PI) * frequency * t)/3 + amplitude * Math.Sin((10.0 * Math.PI) * frequency * t)/5 +
                    amplitude* Math.Sin((14.0 * Math.PI) * frequency * t)/7 + amplitude * Math.Sin((18.0 * Math.PI) * frequency * t)/9;

            }

            return rVal;
        }
        public static double[] GenerateSineWave(
            double frequency,
            double amplitude,
            double sampleClockRate,
            double samplesPerBuffer)
        {
            double deltaT = 1 / sampleClockRate; // sec./samp
            int intSamplesPerBuffer = (int)samplesPerBuffer;

            double[] rVal = new double[intSamplesPerBuffer];

            for (int i = 0; i < intSamplesPerBuffer; i++)
                rVal[i] = amplitude * Math.Sin((2.0 * Math.PI) * frequency * (i * deltaT));

            return rVal;
        }

        public static void InitComboBox(System.Windows.Forms.ComboBox box)
        {
            box.Items.Clear();
            box.Items.AddRange(new object[] {"Sine Wave",
                "Square Wave"});
            box.Sorted = false;
            box.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            box.Text = "Sine Wave";
        }

        private double[] _data;
        private double _resultingSampleClockRate;
        private double _resultingFrequency;
        private double _desiredSampleClockRate;
        private double _samplesPerCycle;
    }
}
