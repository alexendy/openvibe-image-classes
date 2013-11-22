#include "ovpCDisplayImageClasses.h"

#include <cmath>
#include <iostream>
#include <cstdlib>

#if defined TARGET_OS_Linux
  #include <unistd.h>
#endif


// OpenViBE plugin for displaying images from different classes depending on the stimulus received.
// Alexandre Coninx, Imperial COllege London, 2013
// Heavily based on the DisplayCueImage plugin


using namespace OpenViBE;
using namespace Plugins;
using namespace Kernel;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

using namespace OpenViBEToolkit;

using namespace std;

string convertInt(int number)
{
    if (number == 0)
        return "0";
    string temp="";
    string returnvalue="";
    while (number>0)
    {
        temp+=number%10+48;
        number/=10;
    }
    for (unsigned int i=0;i<temp.length();i++)
        returnvalue+=temp[temp.length()-i-1];
    return returnvalue;
}


namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		gboolean DisplayImageClasses_SizeAllocateCallback(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
		{
			reinterpret_cast<CDisplayImageClasses*>(data)->resize((uint32)allocation->width, (uint32)allocation->height);
			return FALSE;
		}

		gboolean DisplayImageClasses_RedrawCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
		{
			reinterpret_cast<CDisplayImageClasses*>(data)->redraw();
			return TRUE;
		}

		CDisplayImageClasses::CDisplayImageClasses(void) :
			m_pBuilderInterface(NULL),
			m_pMainWindow(NULL),
			m_pDrawingArea(NULL),
			//m_pStimulationReaderCallBack(NULL),
			m_bImageRequested(false),
			m_int32RequestedImageID(-1),
			m_bImageDrawn(false),
			m_int32DrawnImageID(-1),
			m_pOriginalPicture(NULL),
			m_pScaledPicture(NULL),
			m_pStimulationsId(NULL),
			m_pImageNames(NULL),
			m_bFullScreen(false),
			m_ui64LastOutputChunkDate(-1),
			m_bError(false)
		{
			//m_pReader[0] = NULL;


			m_oBackgroundColor.pixel = 0;
			m_oBackgroundColor.red = 0;
			m_oBackgroundColor.green = 0;
			m_oBackgroundColor.blue = 0;

			m_oForegroundColor.pixel = 0;
			m_oForegroundColor.red = 0xFFFF;
			m_oForegroundColor.green = 0xFFFF;
			m_oForegroundColor.blue = 0xFFFF;
		}

		boolean CDisplayImageClasses::initialize()
		{
			m_bError=false;

			//>>>> Reading Settings:
			CString l_sSettingValue;

			//Number of Cues:
			m_ui32NumberOfClasses = getStaticBoxContext().getSettingCount() - 6;


			//Do we display the images in full screen?
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, l_sSettingValue);
			m_bFullScreen=(l_sSettingValue==CString("true")?true:false);

			//Clear screen stimulation:
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, l_sSettingValue);
			m_ui64ClearScreenStimulation=getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, l_sSettingValue);

			//Display cross stimulation:
			getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(2, l_sSettingValue);
			m_ui64DisplayCrossStimulation=getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, l_sSettingValue);

			//Number of stimulations by class
       //                 getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(3, l_sSettingValue);
			m_ui32ImagesPerClass = int64(FSettingValueAutoCast(*this->getBoxAlgorithmContext(),3));

			// Image dir
                        getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(4, m_ImageDir);
//			m_ImageDir = l_sSettingValue; // Cast needed ?

			// Image pattern
                        getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(5, m_ImageNameTemplate);
//			m_ImageNameTemplate = l_sSettingValue; // Cast needed ?

			string base_filename = string(m_ImageDir) + string(m_ImageNameTemplate);

			// Compute total number of images
			m_ui32NumberOfImages = m_ui32ImagesPerClass * m_ui32NumberOfClasses;

			//Stimulation ID and images file names for each cue
			m_pImageNames = new CString[m_ui32NumberOfImages];
			m_pStimulationsId = new uint64[m_ui32NumberOfClasses];
			m_pImageDisplayCountPerClass = new uint32[m_ui32NumberOfClasses];
			for(uint32 i=0; i<m_ui32NumberOfClasses; i++)
			{
//				getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(i+6, m_pImageNames[i]);
				getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(i+6, l_sSettingValue);
				m_pStimulationsId[i]=getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, l_sSettingValue);
				m_pImageDisplayCountPerClass[i] = 0;
				for(uint32 j=0; j<m_ui32ImagesPerClass; j++)
				{
					string file = string(base_filename);
					int pos_class = file.find("##CLASS##");
					file.replace(pos_class,9,convertInt(i+1));
					int pos_number = file.find("##NUMBER##");
					file.replace(pos_number,10,convertInt(j+1));
					m_pImageNames[m_ui32ImagesPerClass*i + j] = CString(file.c_str()); // Convert to OpenViBE CString type
				}
			}

			// Load image names ETC
			

			//>>>> Initialisation
			m_oStimulationDecoder.initialize(*this);
			m_oStimulationEncoder.initialize(*this);

			//load the gtk builder interface
			m_pBuilderInterface=gtk_builder_new();
			gtk_builder_add_from_file(m_pBuilderInterface, OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-DisplayImageClasses.ui", NULL);

			if(!m_pBuilderInterface)
			{
				m_bError = true;
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Couldn't load the interface !";
				return false;
			}

			gtk_builder_connect_signals(m_pBuilderInterface, NULL);

			m_pDrawingArea = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "DisplayImageClassesDrawingArea"));
			g_signal_connect(G_OBJECT(m_pDrawingArea), "expose_event", G_CALLBACK(DisplayImageClasses_RedrawCallback), this);
			g_signal_connect(G_OBJECT(m_pDrawingArea), "size-allocate", G_CALLBACK(DisplayImageClasses_SizeAllocateCallback), this);

			//set widget bg color
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oBackgroundColor);

			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oForegroundColor);

			//Load the pictures:
			m_pOriginalPicture = new GdkPixbuf*[m_ui32NumberOfImages];
			m_pScaledPicture = new GdkPixbuf*[m_ui32NumberOfImages];

			for(uint32 i=0; i<m_ui32NumberOfImages; i++)
			{
				m_pOriginalPicture[i] = gdk_pixbuf_new_from_file_at_size(m_pImageNames[i], -1, -1, NULL);
				m_pScaledPicture[i]=0;
				if(!m_pOriginalPicture[i])
				{
					getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Error couldn't load ressource file : " << m_pImageNames[i] << "!\n";
					m_bError = true;
					return false;
				}
			}

			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);

			return true;
		}

		boolean CDisplayImageClasses::uninitialize()
		{
			m_oStimulationDecoder.uninitialize();
			m_oStimulationEncoder.uninitialize();

			//destroy drawing area
			if(m_pDrawingArea)
			{
				gtk_widget_destroy(m_pDrawingArea);
				m_pDrawingArea = NULL;
			}

			// unref the xml file as it's not needed anymore
			if(m_pBuilderInterface) 
			{
				g_object_unref(G_OBJECT(m_pBuilderInterface));
				m_pBuilderInterface=NULL;
			}

			if(m_pStimulationsId) 
			{
				delete[] m_pStimulationsId;
				m_pStimulationsId = NULL;
			}
			if(m_pImageNames)
			{
				delete[] m_pImageNames;
				m_pImageNames = NULL;
			}

			if(m_pOriginalPicture) 
			{
				for(uint32 i=0; i<m_ui32NumberOfImages; i++)
				{
					if(m_pOriginalPicture[i]){ g_object_unref(G_OBJECT(m_pOriginalPicture[i])); }
				}
				delete[] m_pOriginalPicture;
				m_pOriginalPicture = NULL;
			}

			if(m_pScaledPicture) 
			{
				for(uint32 i=0; i<m_ui32NumberOfImages; i++)
				{
					if(m_pScaledPicture[i]){ g_object_unref(G_OBJECT(m_pScaledPicture[i])); }
				}
				delete[] m_pScaledPicture;
				m_pScaledPicture = NULL;
			}

			return true;
		}

		boolean CDisplayImageClasses::processClock(CMessageClock& rMessageClock)
		{
			IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();
			m_oStimulationEncoder.getInputStimulationSet()->clear();

			if(m_bImageDrawn)
			{
				// this is first redraw() for that image or clear screen
				// we send a stimulation to signal it.


				if (m_int32DrawnImageID>=0)
				{
					// it was a image
					m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(
								m_pStimulationsId[m_int32DrawnImageID],
								this->getPlayerContext().getCurrentTime(),
								0);
				}
				else if (m_int32DrawnImageID == -2)
				{
					// it was a cross	
					m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(
								m_ui64DisplayCrossStimulation,
								this->getPlayerContext().getCurrentTime(),
								0);

				}
				else
				{
					// it was a clear_screen
					m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(
								m_ui64ClearScreenStimulation,
								this->getPlayerContext().getCurrentTime(),
								0);
				}

				m_bImageDrawn = false;

				if (m_int32DrawnImageID != m_int32RequestedImageID)
				{
					// We must be late...
					getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "One image may have been skipped => we must be late...\n";
				}
			}

			m_oStimulationEncoder.encodeBuffer(0);
			l_pBoxIO->markOutputAsReadyToSend(0, m_ui64LastOutputChunkDate, this->getPlayerContext().getCurrentTime());
			m_ui64LastOutputChunkDate = this->getPlayerContext().getCurrentTime();

			// We check if some images must be display
			for(uint32 stim = 0; stim < m_oPendingStimulationSet.getStimulationCount() ; )
			{
				uint64 l_ui64StimDate = m_oPendingStimulationSet.getStimulationDate(stim);
				uint64 l_ui64Time = this->getPlayerContext().getCurrentTime();
				if (l_ui64StimDate < l_ui64Time)
				{
					float l_fDelay = (float)(((l_ui64Time - l_ui64StimDate)>> 16) / 65.5360); //delay in ms
					if (l_fDelay>50)
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Image was late: "<< l_fDelay <<" ms \n";


					uint64 l_ui64StimID =   m_oPendingStimulationSet.getStimulationIdentifier(stim);
					getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Trace << "Got stimulation #"<< l_ui64StimID <<" \n";
					if(l_ui64StimID== m_ui64ClearScreenStimulation)
					{
						if (m_bImageRequested)
							getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "One image was skipped => Not enough time between two images!!\n";
						m_bImageRequested = true;
						m_int32RequestedImageID = -1;
					}
					else if(l_ui64StimID== m_ui64DisplayCrossStimulation)
					{
						if (m_bImageRequested)
							getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "One image was skipped => Not enough time between two images!!\n";
						m_bImageRequested = true;
						m_int32RequestedImageID = -2;
					}
					else
					{
						for(uint32 i=0; i<=m_ui32NumberOfClasses; i++)
						{
							if(l_ui64StimID == m_pStimulationsId[i])
							{
								if (m_bImageRequested)
									getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "One image was skipped => Not enough time between two images!!\n";
								if(m_pImageDisplayCountPerClass[i] >= m_ui32ImagesPerClass)
								{
									getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "We have run through all images for class " << i << ", reseting to image 0!\n";
									m_pImageDisplayCountPerClass[i] = 0;
								}
								m_bImageRequested = true;
								m_int32RequestedImageID = i*m_ui32ImagesPerClass+m_pImageDisplayCountPerClass[i];
								m_pImageDisplayCountPerClass[i]++;
								break;
							}
						}
					}

					m_oPendingStimulationSet.removeStimulation(stim);

					if(GTK_WIDGET(m_pDrawingArea)->window)
					{
						gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,NULL,true);
						// it will trigger the callback redraw()
					}

				}
				else
				{
					stim++;
				}
			}

			return true;
		}

		boolean CDisplayImageClasses::processInput(uint32 ui32InputIndex)
		{
			if(m_bError)
			{
				return false;
			}

			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		boolean CDisplayImageClasses::process()
		{
			IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();
		

			// We decode and save the received stimulations.
			for(uint32 input=0; input < getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount(); input++)
			{
				for(uint32 chunk=0; chunk < l_pBoxIO->getInputChunkCount(input); chunk++)
				{
					m_oStimulationDecoder.decode(0,chunk,true);
					if(m_oStimulationDecoder.isHeaderReceived())
					{
						m_ui64LastOutputChunkDate = this->getPlayerContext().getCurrentTime();
						m_oStimulationEncoder.encodeHeader(0);
						l_pBoxIO->markOutputAsReadyToSend(0, 0, m_ui64LastOutputChunkDate);
					}
					if(m_oStimulationDecoder.isBufferReceived())
					{
						for(uint32 stim = 0; stim < m_oStimulationDecoder.getOutputStimulationSet()->getStimulationCount(); stim++)
						{
							uint64 l_ui64StimID =  m_oStimulationDecoder.getOutputStimulationSet()->getStimulationIdentifier(stim);


							boolean l_bAddStim = false;
							if(l_ui64StimID == m_ui64ClearScreenStimulation || l_ui64StimID == m_ui64DisplayCrossStimulation)
								l_bAddStim = true;
							else
								for(uint32 i=0; i<=m_ui32NumberOfClasses; i++)
									if(l_ui64StimID == m_pStimulationsId[i])
									{
										l_bAddStim = true;
										break;
									}

							if (l_bAddStim)
							{
								uint64 l_ui64StimDate =     m_oStimulationDecoder.getOutputStimulationSet()->getStimulationDate(stim);
								uint64 l_ui64StimDuration = m_oStimulationDecoder.getOutputStimulationSet()->getStimulationDuration(stim);

								uint64 l_ui64Time = this->getPlayerContext().getCurrentTime();
								if (l_ui64StimDate < l_ui64Time)
								{
									float l_fDelay = (float)(((l_ui64Time - l_ui64StimDate)>> 16) / 65.5360); //delay in ms
									if (l_fDelay>50)
										getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Stimulation was received late: "<< l_fDelay <<" ms \n";
								}

								if (l_ui64StimDate < l_pBoxIO->getInputChunkStartTime(input, chunk))
								{
									this->getLogManager() << LogLevel_ImportantWarning << "Input Stimulation Date before beginning of the buffer\n";
								}

								m_oPendingStimulationSet.appendStimulation(
											l_ui64StimID,
											l_ui64StimDate,
											l_ui64StimDuration);
							}


						}
					}
					l_pBoxIO->markInputAsDeprecated(input, chunk);
				}
			}
			
			return true;
		}

		//Callback called by GTK
		void CDisplayImageClasses::redraw()
		{
			
			if (m_int32RequestedImageID >= 0)
			{
				drawPicture(m_int32RequestedImageID);
			} else if (m_int32RequestedImageID == -2) {
				drawCross();
			}

			if(m_bImageRequested)
			{
				m_bImageRequested = false;
				m_bImageDrawn = true;
				m_int32DrawnImageID = m_int32RequestedImageID;
			}
			
		}

		void CDisplayImageClasses::drawPicture(OpenViBE::uint32 uint32CueID)
		{
			
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			if(m_bFullScreen)
			{
				gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pScaledPicture[uint32CueID], 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
			}
			else
			{
				gint l_iX = (l_iWindowWidth/2) - gdk_pixbuf_get_width(m_pScaledPicture[uint32CueID])/2;
				gint l_iY = (l_iWindowHeight/2) - gdk_pixbuf_get_height(m_pScaledPicture[uint32CueID])/2;;
				gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pScaledPicture[uint32CueID], 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
			}
			
		}
		void CDisplayImageClasses::clearDisplayZone()
		{
			
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			gdk_draw_rectangle(m_pDrawingArea->window, m_pDrawingArea->style->black_gc, TRUE, 0, 0, l_iWindowWidth, l_iWindowHeight);
			
		}

		void CDisplayImageClasses::drawCross()
		{
			
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			gint hline_x1 = l_iWindowWidth * 0.2;
			gint hline_x2 = l_iWindowWidth * 0.8;
			gint hline_y = l_iWindowHeight * 0.5;

			gint vline_y1 = l_iWindowHeight * 0.2;
			gint vline_y2 = l_iWindowHeight * 0.8;
			gint vline_x = l_iWindowWidth * 0.5;

			gint extra_thickness = 1;

			gdk_draw_rectangle(m_pDrawingArea->window, m_pDrawingArea->style->white_gc, TRUE, hline_x1, hline_y-extra_thickness, hline_x2-hline_x1, 2*extra_thickness);
			gdk_draw_rectangle(m_pDrawingArea->window, m_pDrawingArea->style->white_gc, TRUE, vline_x-extra_thickness, vline_y1, 2*extra_thickness, vline_y2 - vline_y1);
			
		}

		void CDisplayImageClasses::resize(uint32 ui32Width, uint32 ui32Height)
		{
			
			for(uint32 i=0; i<m_ui32NumberOfImages; i++)
			{
				if(m_pScaledPicture[i]){ g_object_unref(G_OBJECT(m_pScaledPicture[i])); }
			}

			if(m_bFullScreen)
			{
				for(uint32 i=0; i<m_ui32NumberOfImages; i++)
				{
					m_pScaledPicture[i] = gdk_pixbuf_scale_simple(m_pOriginalPicture[i], ui32Width, ui32Height, GDK_INTERP_BILINEAR);
				}
			}
			else
			{
				float l_fX = (float)(ui32Width<64?64:ui32Width);
				float l_fY = (float)(ui32Height<64?64:ui32Height);
				for(uint32 i=0; i<m_ui32NumberOfImages; i++)
				{
					float l_fx = (float)gdk_pixbuf_get_width(m_pOriginalPicture[i]);
					float l_fy = (float)gdk_pixbuf_get_height(m_pOriginalPicture[i]);
					if((l_fX/l_fx) < (l_fY/l_fy))
					{
						l_fy = l_fX*l_fy/(3*l_fx);
						l_fx = l_fX/3;
					}
					else
					{
						l_fx = l_fY*l_fx/(3*l_fy);
						l_fy = l_fY/3;
					}
					m_pScaledPicture[i] = gdk_pixbuf_scale_simple(m_pOriginalPicture[i], (int)l_fx, (int)l_fy, GDK_INTERP_BILINEAR);
				}
			}
		
		}
	};
};
