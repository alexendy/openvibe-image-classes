#ifndef __OpenViBEPlugins_SimpleVisualisation_CDisplayImageClasses_H__
#define __OpenViBEPlugins_SimpleVisualisation_CDisplayImageClasses_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <map>
#include <deque>

#define OVP_ClassId_DisplayImageClasses                                            OpenViBE::CIdentifier(0x5050373C, 0x79351882)
#define OVP_ClassId_DisplayImageClassesDesc                                        OpenViBE::CIdentifier(0x22BA0293, 0x3197415D)

// OpenViBE plugin for displaying images from different classes depending on the stimulus received.
// Alexandre Coninx, Imperial COllege London, 2013
// Heavily based on the DisplayCueImage plugin

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{

		class CDisplayImageClasses :
				public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
				//public OpenViBEToolkit::IBoxAlgorithmStimulationInputReaderCallback::ICallback
		{
		public:

			CDisplayImageClasses(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize();
			virtual OpenViBE::boolean uninitialize();
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::uint64 getClockFrequency(void){ return (128LL<<32); }
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			virtual OpenViBE::boolean process();
			virtual void redraw(void);
			virtual void resize(OpenViBE::uint32 ui32Width, OpenViBE::uint32 ui32Height);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_DisplayImageClasses)

			protected:

				//virtual void setStimulationCount(const OpenViBE::uint32 ui32StimulationCount);
				//virtual void setStimulation(const OpenViBE::uint32 ui32StimulationIndex, const OpenViBE::uint64 ui64StimulationIdentifier, const OpenViBE::uint64 ui64StimulationDate);

				//virtual void processState(void);
				virtual void drawPicture(OpenViBE::uint32 uint32CueID);
				virtual void drawCross();
				virtual void clearDisplayZone();

			//The Builder handler used to create the interface
			::GtkBuilder* m_pBuilderInterface;
			::GtkWidget*  m_pMainWindow;
			::GtkWidget*  m_pDrawingArea;

			OpenViBEToolkit::TStimulationDecoder<CDisplayImageClasses> m_oStimulationDecoder;
			OpenViBEToolkit::TStimulationEncoder<CDisplayImageClasses> m_oStimulationEncoder;

			/*penViBE::Kernel::IAlgorithmProxy* m_pStreamEncoder;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IStimulationSet* > ip_pStimulationSet;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMemoryBuffer* > op_pMemoryBuffer;*/

			// For the display of the images:
			OpenViBE::boolean m_bImageRequested;        //when true: a new image must be drawn
			OpenViBE::int32   m_int32RequestedImageID;  //ID of the requested image. -1 => clear the screen

			OpenViBE::boolean m_bImageDrawn;            //when true: the new image has been drawn
			OpenViBE::int32   m_int32DrawnImageID;      //ID of the drawn image. -1 => clear the screen


			::GdkPixbuf** m_pOriginalPicture;
			::GdkPixbuf** m_pScaledPicture;

			::GdkColor m_oBackgroundColor;
			::GdkColor m_oForegroundColor;

			//Settings
			OpenViBE::uint32   m_ui32NumberOfClasses;
			OpenViBE::uint32   m_ui32ImagesPerClass;
			OpenViBE::uint64*  m_pStimulationsId;
			OpenViBE::uint32*  m_pImageDisplayCountPerClass;
			OpenViBE::CString  m_ImageDir;
			OpenViBE::CString  m_ImageNameTemplate;
			OpenViBE::CString* m_pImageNames;
			OpenViBE::uint64   m_ui64ClearScreenStimulation;
			OpenViBE::uint64   m_ui64DisplayCrossStimulation;
			OpenViBE::boolean  m_bFullScreen;

			// Convenience variables
                        OpenViBE::uint32   m_ui32NumberOfImages;


			//Start and end time of the last buffer
			OpenViBE::uint64 m_ui64StartTime;
			OpenViBE::uint64 m_ui64EndTime;
			OpenViBE::uint64 m_ui64LastOutputChunkDate;

			//We save the received stimulations
			OpenViBE::CStimulationSet m_oPendingStimulationSet;

			OpenViBE::boolean m_bError;
		};

		class CDisplayImageClassesListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			virtual OpenViBE::boolean onSettingAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
//				OpenViBE::CString l_sDefaultName = OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/p300-magic-card/bomberman.png";

//				rBox.setSettingDefaultValue(ui32Index, l_sDefaultName.toASCIIString());
//				rBox.setSettingValue(ui32Index, l_sDefaultName.toASCIIString());

				char l_sName[1024];
				sprintf(l_sName, "Stimulation for class %i", (ui32Index-5));
				rBox.setSettingName(ui32Index, l_sName);
				rBox.setSettingType(ui32Index, OV_TypeId_Stimulation);
				sprintf(l_sName, "OVTK_StimulationId_Label_%02X", ui32Index-5);
				rBox.setSettingDefaultValue(ui32Index, l_sName);
				rBox.setSettingValue(ui32Index, l_sName);


				this->checkSettingNames(rBox);
				return true;
			}

			virtual OpenViBE::boolean onSettingRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				this->checkSettingNames(rBox);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);

		private:

			OpenViBE::boolean checkSettingNames(OpenViBE::Kernel::IBox& rBox)
			{
				char l_sName[1024];
				for(OpenViBE::uint32 i=6; i<rBox.getSettingCount()-1; i++)
				{
					sprintf(l_sName, "Stimulation for class %i", (i-5));
					rBox.setSettingName(i, l_sName);
					rBox.setSettingType(i, OV_TypeId_Stimulation);
				}
				return true;
			}
		};

		/**
		 * Plugin's description
		 */
		class CDisplayImageClassesDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Images classes display"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Alexandre Coninx"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Imperial College London"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Display images from different classes"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Display images from different classes based on stimulation received and a fixation cross for OVTK_GDF_Cross_On_Screen. Heavily based on the DisplayCueImage box."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.1"); }
			virtual void release(void)                                   { }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_DisplayImageClasses; }

			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-fullscreen"); }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CDisplayImageClasses(); }
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CDisplayImageClassesListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePF) const
			{
				return ePF == OpenViBE::Kernel::PluginFunctionality_Visualization;
			}

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addInput  ("Stimulations", OV_TypeId_Stimulations);
				rPrototype.addOutput ("Stimulations", OV_TypeId_Stimulations);
				rPrototype.addSetting("Display images in full screen", OV_TypeId_Boolean, "false");
				rPrototype.addSetting("Clear screen stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_VisualStimulationStop");
				rPrototype.addSetting("Display cross stimulation", OV_TypeId_Stimulation, "OVTK_GDF_Cross_On_Screen");
				rPrototype.addSetting("Number of images by class", OV_TypeId_Integer, "20");
				rPrototype.addSetting("Image directory", OV_TypeId_String, "${Path_Data}/plugins/simple-visualisation/classes-images/");
				rPrototype.addSetting("Image name pattern", OV_TypeId_String, "img-##CLASS##-##NUMBER##.png");
				rPrototype.addSetting("Stimulation for class 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rPrototype.addSetting("Stimulation for class 2", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_02");
				rPrototype.addFlag   (OpenViBE::Kernel::BoxFlag_CanAddSetting);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_DisplayImageClassesDesc)
		};
	};
};

#endif
