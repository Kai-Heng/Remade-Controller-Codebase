using NationalInstruments;
using NationalInstruments.Analysis;
using NationalInstruments.Analysis.Conversion;
using NationalInstruments.Analysis.Dsp;
using NationalInstruments.Analysis.Dsp.Filters;
using NationalInstruments.Analysis.Math;
using NationalInstruments.Analysis.Monitoring;
using NationalInstruments.Analysis.SignalGeneration;
using NationalInstruments.Analysis.SpectralMeasurements;
using NationalInstruments.Controls;
using NationalInstruments.Controls.Rendering;
using NationalInstruments.NetworkVariable;
using NationalInstruments.NetworkVariable.WindowsForms;
using NationalInstruments.Tdms;
using NationalInstruments.UI;
using NationalInstruments.UI.WindowsForms;
using NationalInstruments.Visa;
using NationalInstruments.DAQmx;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace PulseGenerator
{
    public partial class Form1 : Form
    {
    
        private NationalInstruments.DAQmx.Task pulseGenerationTask;
        private AnalogSingleChannelWriter writer;

        public Form1()
        {
            InitializeComponent();
            InitializeTask();
        }

        private void InitializeTask()
        {
            string deviceName = "cDAQ1Mod1"; // Replace with your actual device name
            string outputChannel = "ao0"; // Replace with your actual output channel

            pulseGenerationTask = new NationalInstruments.DAQmx.Task();
            pulseGenerationTask.AOChannels.CreateVoltageChannel($"{deviceName}/{outputChannel}", "Pulse Generation",
                -10.0, 10.0, AOVoltageUnits.Volts);

            writer = new AnalogSingleChannelWriter(pulseGenerationTask.Stream);
        }

        private void StartButton_Click(object sender, EventArgs e)
        {
            double pulseAmplitude = 10.0; // Amplitude of the pulse in volts
            double pulseDuration = 0.001; // Pulse duration in seconds
            double pulseDelay = 0.5; // Delay before the pulse in seconds

            int totalSamples = (int)(pulseDuration * 1000.0); // Convert pulse duration to milliseconds

            double[] pulseData = new double[totalSamples + 2];
            pulseData[0] = 0.0; //initial value

            for (int i = 1; i <= totalSamples; i++)
            {
                pulseData[i] = pulseAmplitude; // Pulse amplitude
            }
            pulseData[totalSamples + 1] = 0.0; // Return to initial value

            pulseGenerationTask.Timing.ConfigureSampleClock("", 1000.0, SampleClockActiveEdge.Rising,
                SampleQuantityMode.FiniteSamples, pulseData.Length);

            writer.WriteMultiSample(false, pulseData);

            pulseGenerationTask.Start();
        }

        private void StopButton_Click(object sender, EventArgs e)
        {
            if (pulseGenerationTask != null && pulseGenerationTask.IsDone)
            {
                pulseGenerationTask.Stop();
                pulseGenerationTask.Dispose();
                InitializeTask();

                MessageBox.Show("Pulse generation stopped.");
            }
            else
            {
                MessageBox.Show("No pulse generation task is currently running.");
            }
        }
    }
}
