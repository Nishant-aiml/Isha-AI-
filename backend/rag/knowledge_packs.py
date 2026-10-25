"""
Pre-bundled Domain Knowledge Packs for ISHA AI.
Includes Agriculture, Healthcare, Civic/Law, STEM, Coding/Tech, Daily Finance, and General Science.
"""

from typing import Dict, Any

DOMAIN_PACKS: Dict[str, Dict[str, Any]] = {
    "bharat_core": {
        "title": "Bharat Vidhi & Civic Core (Law & Citizen Rights)",
        "description": "Indian Constitution, legal safeguards, consumer protection, new criminal laws (BNS/BNSS), police FIR procedures, and citizen helplines.",
        "icon": "⚖️",
        "chunks": [
            {
                "id": "bharat_001",
                "title": "National Emergency Helpline Numbers",
                "text": "National Emergency Unified Helpline: 112 (Police, Fire, Ambulance). Police: 100, Fire: 101, Ambulance: 108. Women Helpline (National): 1091 and 181. National Cyber Crime Reporting Portal: 1930. Childline Helpline: 1098. Senior Citizen Helpline (Elderline): 14567. Railway Security Helpline: 139. Disaster Management Helpline: 1078."
            },
            {
                "id": "bharat_002",
                "title": "Filing a Police FIR (First Information Report) & Zero FIR",
                "text": "Under Section 173 Bharatiya Nagarik Suraksha Sanhita (BNSS) / Section 154 CrPC, an officer in charge must register an FIR upon information of a cognizable offence. A citizen has the legal right to receive a signed copy of the FIR immediately and free of cost. If a crime occurred outside police station jurisdiction, police must register a 'Zero FIR' and transfer it later. Refusal to register FIR is punishable under law."
            },
            {
                "id": "bharat_003",
                "title": "Fundamental Rights Guaranteed by Constitution of India",
                "text": "Part III of the Indian Constitution enshrines 6 Fundamental Rights: Right to Equality (Articles 14-18), Right to Freedom (Articles 19-22, including speech and expression), Right against Exploitation (Articles 23-24), Right to Freedom of Religion (Articles 25-28), Cultural and Educational Rights (Articles 29-30), and Right to Constitutional Remedies (Article 32 - enabling citizens to approach Supreme Court directly via habeas corpus, mandamus, prohibition, quo warranto, and certiorari)."
            },
            {
                "id": "bharat_004",
                "title": "Right to Information (RTI Act 2005) Process",
                "text": "Any Indian citizen can request information from a Public Authority under RTI Act 2005. Standard application fee is ₹10 (exempt for Below Poverty Line applicants). The Public Information Officer (PIO) must supply the information within 30 days. If the matter involves life or liberty of a person, information must be provided within 48 hours."
            },
            {
                "id": "bharat_005",
                "title": "Consumer Protection Act 2019 Remedies",
                "text": "Under Consumer Protection Act 2019, consumers can file e-complaints via the e-Daakhil portal without needing a lawyer. District Commissions adjudicate claims up to ₹50 Lakh, State Commissions up to ₹2 Crore, and National Commission above ₹2 Crore. Covers unfair trade practices, misleading advertisements, defective goods, and deficiency in services."
            },
            {
                "id": "bharat_006",
                "title": "Cybercrime Financial Fraud Golden Hour (Helpline 1930)",
                "text": "If victimized by financial cyber fraud (UPI fraud, phishing, credit card scam), immediately dial 1930 within the 'Golden Hour' (first 2-4 hours). The Citizen Financial Cyber Fraud Reporting and Management System (CFCFRMS) coordinates with banks to freeze transferred funds in the fraudster's account before withdrawal. Also lodge complaint on cybercrime.gov.in."
            },
            {
                "id": "bharat_007",
                "title": "Rights of an Arrested Person",
                "text": "Under Article 22 of Constitution and BNSS/CrPC: 1. Police must inform the grounds of arrest immediately. 2. Right to consult and be defended by a legal practitioner of choice. 3. Right to free legal aid via DLSA if indigent. 4. Right to have a friend or relative informed of the arrest. 5. Must be produced before the nearest Judicial Magistrate within 24 hours of arrest (excluding journey time)."
            },
            {
                "id": "bharat_008",
                "title": "Traffic Police Rules & DigiLocker Digital Documents",
                "text": "Under Rule 139 of Central Motor Vehicles Rules, citizens can present Driving License (DL), Registration Certificate (RC), and Insurance electronically via Government DigiLocker or mParivahan apps. Traffic police must accept these as legally equivalent to original physical cards. Police cannot seize physical documents if digital versions are presented."
            }
        ]
    },
    "daily_life_finance": {
        "title": "Daily Life, Finance & Cyber Safety",
        "description": "Practical everyday guidance: UPI fraud prevention, banking security, mutual funds/SIP, income tax slabs, consumer rights, and emergency preparedness.",
        "icon": "💳",
        "chunks": [
            {
                "id": "fin_001",
                "title": "UPI Safety Rule: PIN is ONLY for Paying, NEVER for Receiving",
                "text": "Golden UPI Rule: You NEVER need to enter your UPI PIN to receive money into your bank account. Entering your UPI PIN always authorizes money to leave your account. If a buyer, lottery claim, or government scheme asks you to scan a QR code or enter your UPI PIN to 'receive cashback or money', it is 100% a scam."
            },
            {
                "id": "fin_002",
                "title": "QR Code & Phishing Scams Prevention",
                "text": "Never scan a QR code sent over WhatsApp, SMS, or OLX to receive payments. Legitimate banks and buyers send funds directly using your mobile number or VPA without requiring you to scan anything. Never click on shortened links (bit.ly, tinyurl) claiming your SIM card, electricity bill, or bank account will be blocked unless you verify KYC immediately."
            },
            {
                "id": "fin_003",
                "title": "Systematic Investment Plan (SIP) & Compounding",
                "text": "A Systematic Investment Plan (SIP) invests a fixed sum into mutual funds monthly, benefiting from rupee-cost averaging (buying more units when markets drop, fewer when markets rise). The Rule of 72 estimates years required to double your money: 72 divided by annual interest rate (e.g., at 12% CAGR, investments double every 72/12 = 6 years)."
            },
            {
                "id": "fin_004",
                "title": "Public Provident Fund (PPF) & Sukanya Samriddhi (SSY)",
                "text": "PPF has a 15-year tenure with government-backed sovereign guarantee. Minimum annual investment is ₹500, maximum ₹1,50,000. Falls under EEE tax status (Exempt on investment, Exempt on interest, Exempt on maturity). Sukanya Samriddhi Yojana (SSY) is for girl children under age 10, offering higher interest rates and EEE tax benefits."
            },
            {
                "id": "fin_005",
                "title": "New Income Tax Regime Basics",
                "text": "Under the New Tax Regime, no tax is payable up to ₹3,00,000 income. For salaried individuals, standard deduction is ₹75,000. With Section 87A rebate, resident individuals with total taxable income up to ₹7,00,000 pay zero income tax. Slabs: 0-3L Nil, 3-7L 5%, 7-10L 10%, 10-12L 15%, 12-15L 20%, above 15L 30%."
            },
            {
                "id": "fin_006",
                "title": "Health Insurance Policy Checklist",
                "text": "When buying health insurance, verify: 1. Room rent capping (prefer policies with No Room Rent Capping). 2. Co-payment clause (prefer 0% co-pay). 3. Waiting period for pre-existing diseases (usually 24 to 36 months). 4. Restoration/Recharge benefit (restores sum insured if exhausted). 5. Daycare treatments coverage and network hospital cashless list."
            },
            {
                "id": "fin_007",
                "title": "Consumer Grievance Redressal (National Consumer Helpline 1915)",
                "text": "If a retailer, e-commerce company, airline, or telecom provider delivers defective goods or refuses legitimate refund, call National Consumer Helpline at 1915 or register complaint at consumerhelpline.gov.in. If unresolved, file an e-complaint on e-Daakhil (edaakhil.nic.in) for Consumer Dispute Redressal Commission without legal fees."
            },
            {
                "id": "fin_008",
                "title": "Home Emergency Preparedness & Safe Water",
                "text": "Maintain an emergency grab-and-go kit containing: 3-day supply of drinking water, non-perishable foods, first aid kit (ORS, bandages, paracetamol, antiseptic), torch with spare batteries, power bank, photocopies of ID cards, and emergency medicines. To purify contaminated water: boil vigorously for at least 1 minute, or add 2 drops of chlorine bleach (4-6% sodium hypochlorite) per litre and wait 30 minutes."
            }
        ]
    },
    "general_science_gk": {
        "title": "General Science, Space & World Knowledge",
        "description": "Foundational knowledge: solar system, space exploration (ISRO), Indian history milestones, geography, climate, and inventions.",
        "icon": "🌍",
        "chunks": [
            {
                "id": "gk_001",
                "title": "Solar System Structure and Planetary Facts",
                "text": "The Solar System consists of the Sun and eight planets. Inner terrestrial planets (rocky, dense): Mercury (smallest, closest), Venus (hottest due to runaway CO2 greenhouse effect), Earth (only planet with liquid water), Mars (red planet with iron oxide soil). Outer gas & ice giants: Jupiter (largest, Great Red Spot), Saturn (prominent ring system), Uranus (rotates on side), Neptune (farthest, fastest winds)."
            },
            {
                "id": "gk_002",
                "title": "ISRO Landmark Space Exploration Missions",
                "text": "Indian Space Research Organisation (ISRO) landmarks: Chandrayaan-1 (2008, confirmed water molecules on Moon); Mars Orbiter Mission / Mangalyaan (2013, reached Mars orbit on maiden attempt); Chandrayaan-3 (August 23, 2023, historic soft landing near Moon's south pole with Vikram lander and Pragyan rover); Aditya-L1 (2023, solar observation observatory positioned at Lagrange Point 1); and Gaganyaan (human spaceflight mission)."
            },
            {
                "id": "gk_003",
                "title": "Earth Atmospheric Layers & Ozone Shield",
                "text": "Atmospheric layers from ground up: 1. Troposphere (0-12 km, where all weather occurs, temperature drops with altitude). 2. Stratosphere (12-50 km, houses the protective Ozone layer O3 which absorbs harmful solar UV-B radiation). 3. Mesosphere (50-85 km, coldest layer where meteors burn up). 4. Thermosphere (85-600 km, contains Ionosphere for radio communications and auroras). 5. Exosphere (transitions into outer space vacuum)."
            },
            {
                "id": "gk_004",
                "title": "Human Body Major Organ Systems",
                "text": "Circulatory system: Heart has 4 chambers (right atrium/ventricle pump deoxygenated blood to lungs via pulmonary artery; left atrium/ventricle pump oxygenated blood to body via aorta). Respiratory system: Gas exchange occurs in millions of microscopic alveoli in lungs. Excretory system: Kidneys filter metabolic waste (urea) through functional nephrons. Nervous system: Central (brain & spinal cord) and Peripheral nerves communicate via electrochemical synapses."
            },
            {
                "id": "gk_005",
                "title": "Renewable Energy: Solar, Wind and Green Hydrogen",
                "text": "Solar Photovoltaic (PV) cells convert sunlight directly into DC electricity using silicon semiconductor p-n junctions. Wind turbines convert kinetic air motion into mechanical rotation to drive electric generators. Green Hydrogen is produced by splitting water into hydrogen and oxygen via electrolysis powered exclusively by renewable energy, producing zero greenhouse emissions upon combustion."
            },
            {
                "id": "gk_006",
                "title": "Major Milestones in Indian History",
                "text": "Indus Valley Civilization (~2500 BCE, urban planning, Harappa & Mohenjo-daro). Vedic Period and composition of Upanishads. Maurya Empire (~322-185 BCE, Chandragupta and Ashoka's edicts of Dhamma). Gupta Empire (Golden Age of mathematics and astronomy, Aryabhata). First War of Indian Independence (1857). Mahatma Gandhi's Satyagraha, Non-Cooperation, and Dandi March. Indian Independence on August 15, 1947, and adoption of Constitution on January 26, 1950."
            },
            {
                "id": "gk_007",
                "title": "Geography of India: Major Physical Divisions",
                "text": "India's physiographic divisions: 1. The Northern Mountains (Himalayas, young fold mountains with highest peak Kanchenjunga in India). 2. The Indo-Gangetic Plains (fertile alluvial soil formed by Indus, Ganga, Brahmaputra rivers). 3. The Peninsular Plateau (ancient Deccan plateau bounded by Western and Eastern Ghats). 4. The Thar Desert (arid western Rajasthan). 5. The Coastal Plains and Island groups (Andaman & Nicobar in Bay of Bengal, Lakshadweep in Arabian Sea)."
            },
            {
                "id": "gk_008",
                "title": "Famous Scientific Inventions and Discoveries",
                "text": "Penicillin (first antibiotic, discovered by Alexander Fleming in 1928). Universal Gravitation & Three Laws of Motion (Sir Isaac Newton, 1687). Theory of General and Special Relativity (Albert Einstein, 1905/1915). Periodic Table (Dmitri Mendeleev, 1869). Discovery of Raman Effect (Sir C.V. Raman, 1928, Nobel Prize in Physics). Inoculation/Vaccination concept (Edward Jenner with smallpox, 1796)."
            }
        ]
    },
    "kisan_saathi": {
        "title": "Kisan Saathi (Agriculture, Crops & Farming)",
        "description": "Comprehensive agricultural guidance: crop diseases, soil health, fertilizer dosing, irrigation, weather precautions, livestock, and government schemes.",
        "icon": "🌾",
        "chunks": [
            {
                "id": "kisan_001",
                "title": "Wheat Nitrogen Deficiency & Correction",
                "text": "Nitrogen deficiency in wheat causes yellowing of lower leaves, stunted plant height, and reduced tillering. Correction: Apply 60 kg/ha urea at basal dose and top-dress remaining 60 kg/ha in two splits during crown root initiation (CRI, 21 days) and heading stage. Ensure soil has adequate moisture before urea application."
            },
            {
                "id": "kisan_002",
                "title": "Wheat Rust (Yellow and Brown) Identification & Treatment",
                "text": "Yellow/Stripe Rust appears as bright yellow linear pustules arranged in parallel lines on leaf blades. Brown/Leaf rust appears as scattered circular orange-brown pustules. Spray Propiconazole 25% EC at 1 ml/litre or Tebuconazole 25.9% EC at 1 ml/litre immediately upon sighting. Repeat in 15 days if infection persists."
            },
            {
                "id": "kisan_003",
                "title": "Rice Blast Disease Management",
                "text": "Rice Blast (Magnaporthe oryzae) manifests as spindle-shaped or diamond lesions with greyish centres and brown margins on leaves, collars, and neck. Spray Tricyclazole 75% WP at 0.6 g/litre water or Isoprothiolane 40% EC at 1.5 ml/litre. Avoid excessive nitrogen fertilizer during cloudy or humid weather."
            },
            {
                "id": "kisan_004",
                "title": "Phosphorus and Potassium Deficiencies",
                "text": "Phosphorus deficiency shows purplish or dark bronze discoloration on leaf edges and stunted roots; correct by applying Diammonium Phosphate (DAP) at 100 kg/ha at sowing. Potassium deficiency causes burning/scorching along leaf tips and margins; correct with MOP (Muriate of Potash) before flowering stage."
            },
            {
                "id": "kisan_005",
                "title": "Cotton Pink Bollworm Control",
                "text": "Pink bollworm damages flower buds ('rosette flowers') and seeds inside developing bolls. Install pheromone traps at 5 traps/acre for monitoring. If catches exceed 8 moths/trap for 3 consecutive days, spray Profenofos 50% EC at 2 ml/litre or Chlorantraniliprole 18.5% SC at 0.3 ml/litre."
            },
            {
                "id": "kisan_006",
                "title": "Drip Irrigation Water Savings & Soil Testing",
                "text": "Drip irrigation delivers water directly to plant root zones, saving 40-60% water over flood irrigation while boosting crop yields by 20-30%. Soil pH between 6.0 and 7.5 is optimal for nutrient uptake. Apply agricultural lime (CaCO3) for acidic soils (pH < 6.0) and gypsum for alkaline soils (pH > 8.2)."
            },
            {
                "id": "kisan_007",
                "title": "PM-Kisan & Pradhan Mantri Fasal Bima Yojana",
                "text": "Under PM-KISAN, eligible landholding farmer families receive direct income support of ₹6,000 per year in three equal instalments of ₹2,000 via DBT. Under PMFBY, premium rates are 2% for Kharif food/oilseeds, 1.5% for Rabi crops, and 5% for annual commercial/horticultural crops, covering post-harvest and localized calamities."
            },
            {
                "id": "kisan_008",
                "title": "Organic Pest Repellents (Neemastra & Bramhastra)",
                "text": "Neemastra formula: 5 kg crushed neem leaves + 5 litres cow urine + 2 kg cow dung fermented in 100 litres water for 48 hours; controls sucking pests and caterpillars. Bramhastra: Boil crushed neem, custard apple, papaya, and guava leaves in cow urine; spray at 30 ml per litre for borers."
            },
            {
                "id": "kisan_009",
                "title": "Mustard Aphid (Lipaphis erysimi) Control",
                "text": "Mustard aphids suck sap from inflorescences, leaves, and pods, curling leaves and stunting seed formation. Economic threshold level (ETL) is 25-30 aphids per 10 cm terminal shoot. Spray Dimethoate 30% EC at 1.7 ml/litre or Thiamethoxam 25% WG at 0.2 g/litre. Spray late afternoon to protect pollinator bees."
            },
            {
                "id": "kisan_010",
                "title": "Millets (Shree Anna) Cultivation & Water Efficiency",
                "text": "Millets (Bajra, Jowar, Ragi, Kodo, Kutki) require 70% less water than rice and mature in 70-100 days. They thrive in poor soils, endure temperatures up to 45°C, and are naturally gluten-free with high iron and calcium content. Seed rate for Ragi is 4-5 kg/acre; apply 40:20:20 kg NPK per hectare."
            },
            {
                "id": "kisan_011",
                "title": "Soil Micronutrient Management: Zinc and Boron",
                "text": "Zinc deficiency causes 'Khaira' disease in paddy (chlorotic patches with reddish-brown pigmentation). Apply Zinc Sulphate (ZnSO4 21%) at 25 kg/ha to soil or spray 0.5% ZnSO4 with 0.25% slaked lime. Boron deficiency causes fruit cracking in tomato and pomegranate; spray 0.2% Solubor at flowering."
            },
            {
                "id": "kisan_012",
                "title": "Dairy Cattle Mastitis & Lumpy Skin Disease Prevention",
                "text": "Bovine Mastitis (udder infection) causes swollen, hot udders and curdled or blood-tinged milk. Clean teats with 0.5% povidone-iodine dip after milking and infuse intramammary antibiotic under veterinary supervision. For Lumpy Skin Disease (LSD), isolate infected cattle, spray vector repellents, and administer Goat Pox vaccine (heterologous protection)."
            },
            {
                "id": "kisan_013",
                "title": "Kisan Credit Card (KCC) Scheme Benefits",
                "text": "Kisan Credit Card provides farmers with hassle-free credit up to ₹3,00,000 at a baseline 7% interest rate. With prompt repayment incentive of 3%, the effective interest rate drops to only 4% per annum. No collateral required for loans up to ₹1,60,000. Covers post-harvest expenses, crop cultivation, and cattle maintenance."
            },
            {
                "id": "kisan_014",
                "title": "Grain Storage & Pest Protection",
                "text": "Dry harvested grains to below 10-12% moisture content before bagging to prevent fungal aflatoxins and weevil infestation. Clean grain bins thoroughly and line bottom with dried neem leaves. For sealed silos, use Aluminium Phosphide tablets (1 tablet per tonne) strictly under airtight hermetic seal for 7 days."
            }
        ]
    },
    "shiksha": {
        "title": "Shiksha STEM & Reasoning",
        "description": "Scientific constants, physics laws, calculus, biological pathways, chemistry trends, and reasoning fundamentals.",
        "icon": "🎓",
        "chunks": [
            {
                "id": "shiksha_001",
                "title": "Newton's Laws of Motion & Momentum",
                "text": "Newton's First Law (Law of Inertia): An object remains at rest or in uniform motion along a straight line unless acted upon by an external net force. Second Law: Net force equals the rate of change of linear momentum F = dp/dt = m*a. Third Law: For every action force, there is an equal and opposite reaction force acting on different bodies simultaneously."
            },
            {
                "id": "shiksha_002",
                "title": "Thermodynamics Laws & Carnot Efficiency",
                "text": "Zeroth Law: Defines temperature via thermal equilibrium. First Law (Conservation of Energy): ΔU = Q - W. Second Law: Total entropy of an isolated system never decreases; heat cannot spontaneously flow from cooler to hotter body without work. Carnot engine maximum efficiency: η = 1 - (T_cold / T_hot), where temperatures are in Kelvin."
            },
            {
                "id": "shiksha_003",
                "title": "Photosynthesis Light and Dark Reactions",
                "text": "Photosynthesis equation: 6CO2 + 6H2O + light -> C6H12O6 + 6O2. Light reactions occur in thylakoid membranes, producing ATP and NADPH via photophosphorylation while releasing oxygen from photolysis of water. Dark reactions (Calvin Cycle) occur in the stroma, where RuBisCO enzyme fixes CO2 into 3-PGA, synthesizing glyceraldehyde-3-phosphate (G3P)."
            },
            {
                "id": "shiksha_004",
                "title": "Fundamental Calculus Derivatives and Integrals",
                "text": "Standard derivative rules: d/dx(x^n) = n*x^(n-1); d/dx(sin x) = cos x; d/dx(cos x) = -sin x; d/dx(e^x) = e^x; d/dx(ln x) = 1/x. Product rule: (u*v)' = u'v + uv'. Chain rule: d/dx[f(g(x))] = f'(g(x))*g'(x). Fundamental Theorem of Calculus connects differentiation and integration: ∫ from a to b of f(x) dx = F(b) - F(a)."
            },
            {
                "id": "shiksha_005",
                "title": "Ohm's Law, Resistance & Power in Electrical Circuits",
                "text": "Ohm's Law states current I is proportional to potential difference V: V = I * R. Series resistance: R_total = R1 + R2 + R3. Parallel resistance: 1/R_total = 1/R1 + 1/R2 + 1/R3. Electrical power dissipated: P = V * I = I^2 * R = V^2 / R. Electrical energy is measured in kilowatt-hours (1 kWh = 3.6 × 10^6 Joules)."
            },
            {
                "id": "shiksha_006",
                "title": "Periodic Table Periodic Trends",
                "text": "Electronegativity increases across a period (left to right) and decreases down a group (Fluorine is most electronegative). Atomic radius decreases across a period due to increasing effective nuclear charge, and increases down a group due to addition of electron shells. Ionization Energy increases across a period and decreases down a group."
            },
            {
                "id": "shiksha_007",
                "title": "Optics: Snell's Law and Total Internal Reflection",
                "text": "Snell's Law of refraction: n1 * sin(θ1) = n2 * sin(θ2), where n is refractive index. Total Internal Reflection (TIR) occurs when light travels from denser to rarer medium (n1 > n2) at an angle of incidence greater than critical angle θc = arcsin(n2/n1). TIR enables optical fiber communications, endoscopes, and diamond brilliance."
            },
            {
                "id": "shiksha_008",
                "title": "Work, Energy and Power Formulas",
                "text": "Work done W = F * d * cos(θ). Kinetic Energy KE = 1/2 * m * v^2. Gravitational Potential Energy PE = m * g * h. Work-Energy Theorem states net work done equals change in kinetic energy: W_net = ΔKE. Law of Conservation of Energy: Total mechanical energy E = KE + PE remains constant in conservative force fields."
            }
        ]
    },
    "swasthya": {
        "title": "Swasthya Rakshak (Health, First Aid & Emergency)",
        "description": "Essential medical guidance: first aid, pediatric dosage guidelines, disease prevention, stroke/heart attack emergency, and public health schemes.",
        "icon": "🩺",
        "chunks": [
            {
                "id": "swasthya_001",
                "title": "Pediatric Paracetamol Dosage Protocol",
                "text": "Paracetamol dosage for children is strictly weight-based: 10 to 15 mg per kg body weight per dose, administered every 4 to 6 hours as needed. Maximum 4 doses (or 60 mg/kg) within 24 hours. Always use an oral dosing syringe, never a household teaspoon. Never combine with other multi-symptom cold medicines containing paracetamol."
            },
            {
                "id": "swasthya_002",
                "title": "Diarrhea & Dehydration ORS Preparation",
                "text": "Oral Rehydration Salts (ORS) is the cornerstone treatment for acute diarrhea. Dissolve 1 full WHO-formula ORS sachet into exactly 1 litre of clean boiled and cooled drinking water. Stir until completely dissolved. Give frequent small sips. Accompany with Zinc supplement tablets (20 mg daily for children >6 months, 10 mg for <6 months) for 14 continuous days."
            },
            {
                "id": "swasthya_003",
                "title": "First Aid for Burns & Scalds",
                "text": "Immediately immerse the burned area under cold, gentle running water for at least 20 minutes to halt thermal tissue destruction. Remove tight jewellery before swelling occurs. Cover loosely with sterile cling film or clean non-adherent cloth. DO NOT apply ice, toothpastes, turmeric, butter, or oil. Seek urgent medical attention if the burn exceeds the size of the victim's palm."
            },
            {
                "id": "swasthya_004",
                "title": "Dengue Warning Signs & Monitoring",
                "text": "Critical phase of Dengue occurs between days 3 and 7 when fever drops. Danger signs requiring immediate hospitalization: severe persistent abdominal pain, recurrent vomiting, bleeding gums or nose, mucosal hemorrhage, extreme lethargy, sudden drop in platelet count accompanied by rise in hematocrit. Avoid Aspirin and Ibuprofen; only use Paracetamol."
            },
            {
                "id": "swasthya_005",
                "title": "Adult Hands-Only CPR Protocol",
                "text": "If an adult collapses and is unresponsive and not breathing normally: 1. Call emergency services (112 / 108) immediately. 2. Position the heel of your hand on the centre of the patient's chest. 3. Push hard and fast at a rate of 100 to 120 compressions per minute to a depth of 5-6 cm (about 2 inches). Allow complete chest recoil between each push."
            },
            {
                "id": "swasthya_006",
                "title": "Hypertension & Diabetes Dietary Management",
                "text": "Hypertension: Restrict dietary sodium to less than 2 grams per day (under 5 grams of table salt). Increase dietary potassium through bananas, green leafy vegetables, and lentils. Diabetes: Prioritize high-fiber complex carbohydrates (whole grains, millets), avoid refined sugars and sugary beverages, and engage in minimum 150 minutes of moderate aerobic exercise weekly."
            },
            {
                "id": "swasthya_007",
                "title": "Ayushman Bharat PM-JAY Scheme Details",
                "text": "Ayushman Bharat Pradhan Mantri Jan Arogya Yojana (PM-JAY) provides cashless secondary and tertiary hospitalization cover of up to ₹5,00,000 per family per year across empaneled public and private hospitals across India. Covers pre-existing conditions from day one, including pre- and post-hospitalization expenses for up to 15 days."
            },
            {
                "id": "swasthya_008",
                "title": "Snakebite First Aid (Do's and Don'ts)",
                "text": "DO NOT tie tight tourniquets, do not suck venom, and do not make cuts on the bite mark. DO: Keep patient calm, immobilize the bitten limb using a splint or sling at heart level, remove rings and constrictive clothing, and rush immediately to a hospital with Anti-Snake Venom (ASV). Polyvalent ASV neutralizes Cobra, Krait, Russell's Viper, and Saw-scaled Viper venoms."
            },
            {
                "id": "swasthya_009",
                "title": "Stroke Recognition (F.A.S.T. Protocol)",
                "text": "Recognize acute stroke using FAST: F (Face drooping on one side), A (Arm weakness when raising both arms), S (Speech difficulty, slurring, inability to repeat simple sentence), T (Time to call 108 immediately). Golden window for intravenous thrombolysis (tissue plasminogen activator tPA) is within 4.5 hours of symptom onset."
            },
            {
                "id": "swasthya_010",
                "title": "Heart Attack Emergency Action",
                "text": "Symptoms: Crushing central chest pain or heaviness lasting >10 minutes, radiating to left shoulder, jaw, back, accompanied by sweating, nausea, shortness of breath. Action: Have patient rest comfortably, call 108, give 300 mg soluble Aspirin/Disprin to chew immediately (unless allergic or actively bleeding), and prepare for CPR if responsiveness is lost."
            },
            {
                "id": "swasthya_011",
                "title": "Dog Bite & Rabies Post-Exposure Prophylaxis (PEP)",
                "text": "Immediately wash the animal bite wound thoroughly under vigorous running water with soap for at least 15 minutes. Apply povidone-iodine or alcohol. DO NOT stitch the wound primarily. Administer Anti-Rabies Vaccine (ARV) on Days 0, 3, 7, and 28. For Category III bites (broken skin or bleeding), Rabies Immunoglobulin (RIG) must be infiltrated into the wound."
            },
            {
                "id": "swasthya_012",
                "title": "Tuberculosis (TB) Symptoms & Treatment (DOTS)",
                "text": "Suspect pulmonary TB if cough lasts >2 weeks, accompanied by fever in evening, night sweats, weight loss, and hemoptysis (blood in sputum). Diagnosis via Sputum smear microscopy and CBNAAT/GeneXpert. Government provides free 6-month Fixed-Dose Combination (FDC) treatment under National TB Elimination Program (NTEP) plus ₹500/month Nikshay Poshan financial aid."
            }
        ]
    },
    "tech_coding": {
        "title": "Tech & Coding (Programming, DSA & Computer Science)",
        "description": "Essential software development guidance: Python, Java, C++, algorithms, data structures, Git commands, Linux, SQL databases, and AI concepts.",
        "icon": "💻",
        "chunks": [
            {
                "id": "tech_001",
                "title": "Python Programming Fundamentals",
                "text": "Python is an interpreted, high-level, dynamically-typed language. Key idioms: List comprehensions [x**2 for x in nums if x % 2 == 0]; dictionary comprehensions; Generators with 'yield' for memory-efficient lazy evaluation; Context managers 'with open(...) as f:' for safe resource disposal; '*args' and '**kwargs' for variable arguments; and '@decorator' for modifying function behavior."
            },
            {
                "id": "tech_002",
                "title": "Java Object-Oriented Programming (OOP) Pillars",
                "text": "Four pillars of Java OOP: 1. Encapsulation (bundling data with private fields and public getters/setters). 2. Inheritance ('extends' for code reuse and hierarchical classification). 3. Polymorphism (Method Overloading compile-time, Method Overriding runtime with @Override). 4. Abstraction ('abstract' classes and 'interface' contracts defining behavior without implementation details)."
            },
            {
                "id": "tech_003",
                "title": "C++ Memory Management and Smart Pointers",
                "text": "In C++, heap allocation with raw 'new'/'delete' risks memory leaks. Modern C++ (C++11/14/17) enforces RAII (Resource Acquisition Is Initialization) via smart pointers in <memory>: std::unique_ptr for exclusive ownership (zero overhead, move-only); std::shared_ptr for reference-counted shared ownership; and std::weak_ptr to break cyclic dependencies."
            },
            {
                "id": "tech_004",
                "title": "Data Structures: Arrays vs Linked Lists",
                "text": "Arrays provide O(1) random access by index due to contiguous memory allocation, but resizing and middle insertions/deletions take O(n). Singly Linked Lists store data and next pointer, allowing O(1) insertions/deletions at known nodes, but require O(n) sequential traversal to find elements and incur pointer memory overhead."
            },
            {
                "id": "tech_005",
                "title": "Big-O Time Complexity Analysis",
                "text": "Big-O characterizes algorithm scalability as input n grows. Constant O(1): Hash table lookup, array index. Logarithmic O(log n): Binary search on sorted array. Linear O(n): Single loop scan. Linearithmic O(n log n): Efficient sorting (MergeSort, QuickSort average, HeapSort). Quadratic O(n^2): Nested loops (BubbleSort, SelectionSort). Exponential O(2^n): Recursive Fibonacci."
            },
            {
                "id": "tech_006",
                "title": "SQL Relational Database Queries and JOINs",
                "text": "INNER JOIN returns rows where condition matches in both tables. LEFT JOIN returns all rows from left table plus matched rows from right table (NULL if no match). RIGHT JOIN returns all rows from right table. GROUP BY aggregates rows via COUNT, SUM, AVG. HAVING filters aggregated results (unlike WHERE which filters before grouping). Indexes speed up SELECT lookups at cost of slower writes."
            },
            {
                "id": "tech_007",
                "title": "Git Essential Workflow & Branching",
                "text": "Initialize repo: git init. Clone remote: git clone <url>. Create & switch branch: git checkout -b feature-branch (or git switch -c). Stage files: git add <files>. Commit changes: git commit -m 'Descriptive message'. Fetch & merge remote: git pull origin main. Push branch: git push -u origin feature-branch. Save temporary work without committing: git stash."
            },
            {
                "id": "tech_008",
                "title": "Linux Shell Commands for Developers",
                "text": "Search text in files: grep -rn 'pattern' ./dir. Search files by name: find . -name '*.kt'. Stream editing: sed 's/foo/bar/g' file. Column processing: awk '{print $1, $3}' file. File permissions: chmod 755 script.sh (rwxr-xr-x). Check running processes: ps aux | grep app, or htop. View disk usage: df -h (filesystems), du -sh * (directory sizes)."
            },
            {
                "id": "tech_009",
                "title": "REST API Architecture & HTTP Status Codes",
                "text": "REST operates over HTTP with standard verbs: GET (read resource), POST (create resource), PUT (replace resource), PATCH (partial update), DELETE (remove resource). Status code categories: 2xx Success (200 OK, 201 Created), 3xx Redirection (301 Moved, 304 Not Modified), 4xx Client Error (400 Bad Request, 401 Unauthorized, 403 Forbidden, 404 Not Found), 5xx Server Error (500 Internal Server Error, 502 Bad Gateway)."
            },
            {
                "id": "tech_010",
                "title": "Artificial Intelligence, LLMs and Quantization",
                "text": "Large Language Models (LLMs) are autoregressive Transformer neural networks trained on predicting next tokens using Self-Attention. Quantization reduces model precision from 16-bit float (FP16) to 4-bit integer (e.g. Q4_K_M in GGUF format), shrinking memory footprint by 70% (e.g. 1.5B parameters fits in ~700MB RAM) to enable on-device inference on mobile phones."
            }
        ]
    }
}
